#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Guid/FileInfo.h>
#include "boot_info.h"

// カーネルエントリーポイントの型定義
typedef void (*KernelEntry)(BOOT_INFO *BootInfo);

EFI_STATUS
EFIAPI
LiteCoreBootManagerMain(IN EFI_HANDLE ImageHandle,
			IN EFI_SYSTEM_TABLE *SystemTable) {
	EFI_STATUS Status;
	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage = NULL;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem = NULL;
	EFI_FILE_PROTOCOL *Root = NULL;
	EFI_FILE_PROTOCOL *KernelFile = NULL;
	UINTN FileInfoSize;
	EFI_FILE_INFO *FileInfo = NULL;
	UINTN KernelSize;
	VOID *KernelBuffer = NULL;
	KernelEntry EntryPoint;
	BOOT_INFO BootInfo;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;

	Print(L"### LiteCoreBootManager ###\n");
	Print(L"Loading kernel...\n");

	// LoadedImageプロトコルを取得
	Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid,
				     (VOID **)&LoadedImage);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to get LoadedImage protocol: %r\n",
		      Status);
		return Status;
	}

	// SimpleFileSystemプロトコルを取得
	Status = gBS->HandleProtocol(LoadedImage->DeviceHandle,
				     &gEfiSimpleFileSystemProtocolGuid,
				     (VOID **)&FileSystem);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to get FileSystem protocol: %r\n",
		      Status);
		return Status;
	}

	// ルートディレクトリを開く
	Status = FileSystem->OpenVolume(FileSystem, &Root);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to open root volume: %r\n", Status);
		return Status;
	}

	// kernel.binを開く
	Status = Root->Open(Root, &KernelFile, L"kernel.bin",
			    EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to open kernel.bin: %r\n", Status);
		Root->Close(Root);
		return Status;
	}

	Print(L"Kernel file opened\n");

	// ファイルサイズを取得
	FileInfoSize = 0;
	Status = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid,
				     &FileInfoSize, NULL);

	FileInfo = AllocatePool(FileInfoSize);
	if (FileInfo == NULL) {
		Print(L"Error: Failed to allocate memory for FileInfo\n");
		KernelFile->Close(KernelFile);
		Root->Close(Root);
		return EFI_OUT_OF_RESOURCES;
	}

	Status = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid,
				     &FileInfoSize, FileInfo);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to get file info: %r\n", Status);
		FreePool(FileInfo);
		KernelFile->Close(KernelFile);
		Root->Close(Root);
		return Status;
	}

	KernelSize = (UINTN)FileInfo->FileSize;
	Print(L"Kernel size: %lu bytes\n", KernelSize);
	FreePool(FileInfo);

	// カーネルを0x10000番地にロード
	UINTN KernelPages = (KernelSize + 0xFFF) / 0x1000;
	EFI_PHYSICAL_ADDRESS KernelAddress = 0x10000;

	Status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, KernelPages,
				    &KernelAddress);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to allocate memory at 0x10000: %r\n",
		      Status);
		KernelFile->Close(KernelFile);
		Root->Close(Root);
		return Status;
	}

	KernelBuffer = (VOID *)KernelAddress;

	// カーネルを読み込む
	Status = KernelFile->Read(KernelFile, &KernelSize, KernelBuffer);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to read kernel: %r\n", Status);
		gBS->FreePages(KernelAddress, KernelPages);
		KernelFile->Close(KernelFile);
		Root->Close(Root);
		return Status;
	}

	Print(L"Kernel loaded at 0x%lx\n", KernelAddress);

	// ファイルをクローズ
	KernelFile->Close(KernelFile);
	Root->Close(Root);

	// GraphicsOutputプロトコルを取得（オプション）
	Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL,
				     (VOID **)&Gop);
	if (!EFI_ERROR(Status) && Gop != NULL) {
		BootInfo.FramebufferBase = Gop->Mode->FrameBufferBase;
		BootInfo.FramebufferSize = Gop->Mode->FrameBufferSize;
		BootInfo.HorizontalResolution =
			Gop->Mode->Info->HorizontalResolution;
		BootInfo.VerticalResolution =
			Gop->Mode->Info->VerticalResolution;
		BootInfo.PixelsPerScanLine = Gop->Mode->Info->PixelsPerScanLine;
		Print(L"Framebuffer: %dx%d at 0x%lx\n",
		      BootInfo.HorizontalResolution,
		      BootInfo.VerticalResolution, BootInfo.FramebufferBase);
	} else {
		BootInfo.FramebufferBase = 0;
		BootInfo.FramebufferSize = 0;
		Print(L"No graphics output available (serial console mode)\n");
	}

	Print(L"Exiting boot services...\n");

	// ブートサービスを終了
	UINTN MapKey;
	UINTN MapSize = 0;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;

	// メモリマップのサイズを取得
	Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey,
				   &DescriptorSize, &DescriptorVersion);
	MapSize += 2 * DescriptorSize;
	MemoryMap = AllocatePool(MapSize);

	Status = gBS->GetMemoryMap(&MapSize, MemoryMap, &MapKey,
				   &DescriptorSize, &DescriptorVersion);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to get memory map: %r\n", Status);
		return Status;
	}

	// ブート情報を設定
	BootInfo.MemoryMap = MemoryMap;
	BootInfo.MemoryMapSize = MapSize;
	BootInfo.MemoryMapDescriptorSize = DescriptorSize;
	BootInfo.MemoryMapDescriptorVersion = DescriptorVersion;

	Status = gBS->ExitBootServices(ImageHandle, MapKey);
	if (EFI_ERROR(Status)) {
		Print(L"Error: Failed to exit boot services: %r\n", Status);
		return Status;
	}

	// カーネルにジャンプ（ブート情報を渡す）
	EntryPoint = (KernelEntry)KernelAddress;
	EntryPoint(&BootInfo);

	// ここには到達しないはず
	return EFI_SUCCESS;
}
