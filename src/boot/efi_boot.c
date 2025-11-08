#include <efi.h>
#include <efilib.h>

// 32bitカーネルへのジャンプ用スタブ（64bit → 32bit切り替え）
// このコードはGDTを設定し、32bitモードに切り替えてからカーネルにジャンプする
__attribute__((section(".text.stub")))
static const unsigned char jump_stub[] = {
    // GDT設定
    0x48, 0xb8,                                     // movabs rax, gdt_ptr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // (GDTアドレス、後で書き換え)
    0x0f, 0x01, 0x10,                               // lgdt [rax]
    
    // 32bit互換モードに切り替え
    0x6a, 0x08,                                     // push 0x08 (32bit code segment)
    0x48, 0xb8,                                     // movabs rax, kernel_addr
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // (カーネルアドレス、後で書き換え)
    0x50,                                           // push rax
    0x48, 0xcb,                                     // retfq (far return)
};

// 32bit用GDT
struct {
    UINT16 limit;
    UINT64 base;
} __attribute__((packed)) gdt_ptr;

struct {
    UINT64 null;
    UINT64 code32;
    UINT64 data32;
} __attribute__((packed)) gdt = {
    0x0000000000000000, // NULL descriptor
    0x00CF9A000000FFFF, // 32bit code: base=0, limit=0xFFFFF, P=1, DPL=0, S=1, Type=0xA
    0x00CF92000000FFFF, // 32bit data: base=0, limit=0xFFFFF, P=1, DPL=0, S=1, Type=0x2
};

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *KernelFile;
    VOID *KernelBuffer = NULL;
    UINTN KernelSize;
    EFI_PHYSICAL_ADDRESS KernelAddr = 0x100000; // 1MB

    InitializeLib(ImageHandle, SystemTable);

    Print(L"LiteCore UEFI Bootloader\n");
    Print(L"Loading kernel...\n");

    // LoadedImageプロトコルを取得
    Status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle,
                               &LoadedImageProtocol, (VOID **)&LoadedImage);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to get LoadedImage protocol (0x%x)\n", Status);
        return Status;
    }

    // ファイルシステムプロトコルを取得
    Status = uefi_call_wrapper(BS->HandleProtocol, 3,
                               LoadedImage->DeviceHandle,
                               &FileSystemProtocol, (VOID **)&FileSystem);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to get FileSystem protocol (0x%x)\n", Status);
        return Status;
    }

    // ルートディレクトリを開く
    Status = uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to open root directory (0x%x)\n", Status);
        return Status;
    }

    // kernel.binを開く
    Status = uefi_call_wrapper(Root->Open, 5, Root, &KernelFile,
                               L"kernel.bin", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to open kernel.bin (0x%x)\n", Status);
        return Status;
    }

    // ファイルサイズを取得
    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoSize = 1024;
    FileInfo = AllocatePool(FileInfoSize);

    Status = uefi_call_wrapper(KernelFile->GetInfo, 4, KernelFile,
                               &GenericFileInfo, &FileInfoSize, FileInfo);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to get file info (0x%x)\n", Status);
        FreePool(FileInfo);
        return Status;
    }

    KernelSize = FileInfo->FileSize;
    Print(L"Kernel size: %lu bytes\n", KernelSize);
    FreePool(FileInfo);

    // カーネル用のメモリを割り当て
    Status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress,
                               EfiLoaderData, (KernelSize + 0xFFF) / 0x1000,
                               &KernelAddr);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to allocate memory for kernel (0x%x)\n", Status);
        return Status;
    }

    KernelBuffer = (VOID *)KernelAddr;

    // カーネルを読み込み
    Status = uefi_call_wrapper(KernelFile->Read, 3, KernelFile, &KernelSize,
                               KernelBuffer);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to read kernel (0x%x)\n", Status);
        return Status;
    }

    uefi_call_wrapper(KernelFile->Close, 1, KernelFile);
    uefi_call_wrapper(Root->Close, 1, Root);

    Print(L"Kernel loaded at 0x%lx\n", KernelAddr);
    Print(L"Preparing to jump to 32-bit kernel...\n");
    
    // GDTポインタを設定
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base = (UINT64)&gdt;
    
    Print(L"Press any key to continue...\n");
    EFI_INPUT_KEY Key;
    while (uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key) != EFI_SUCCESS);

    // ブートサービスを終了
    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MemoryMapSize = 0;
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize,
                               MemoryMap, &MapKey, &DescriptorSize,
                               &DescriptorVersion);

    MemoryMapSize += 2 * DescriptorSize;
    MemoryMap = AllocatePool(MemoryMapSize);

    Status = uefi_call_wrapper(BS->GetMemoryMap, 5, &MemoryMapSize,
                               MemoryMap, &MapKey, &DescriptorSize,
                               &DescriptorVersion);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to get memory map (0x%x)\n", Status);
        return Status;
    }

    Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, MapKey);
    if (EFI_ERROR(Status)) {
        Print(L"Error: Failed to exit boot services (0x%x)\n", Status);
        return Status;
    }

    // 32bitカーネルにジャンプ
    // 注意: これは簡略化されたバージョンで、実際には64bit→32bit切り替えスタブが必要
    // 今は直接ジャンプを試みる
    void (*kernel_entry)(void) = (void (*)(void))KernelAddr;
    kernel_entry();

    // ここには到達しない
    return EFI_SUCCESS;
}
