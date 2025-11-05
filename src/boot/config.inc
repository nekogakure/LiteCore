KERNEL_OFFSET EQU 0x10000       ; カーネルをロードするアドレス
SECTOR_COUNT  EQU 65            ; 読み込むセクタの数
START_SECTOR  EQU 2             ; 開始するセクタ番号
CYLINDER_NUM  EQU 0             ; シリンダ番号
HEAD_NUM      EQU 0             ; ヘッド番号
CODE_SEGMENT  EQU 0x08          ; コードセグメント
DATA_SEGMENT  EQU 0x10          ; データセメント
