#define WRITEBUFSIZE 1024
/* ファイル読み込みの状態を表す構造体 */
typedef struct{
	WORD fd;
	LONG TADRecode;
	LONG LinkRecode;
	LONG offset; /* 現在のオフセット */
				 /* 可変長セグメントの時：データ本体の最初（データ長の後） */
				 /* 固定長セグメントの時：その文字のところ */
	/* ここより読み込み用データ */
	LONG SegOffset; /* セグメントの最初のオフセット */
	LONG NextOffset; /* 次のセグメントのオフセット */
	BYTE ID; /* セグメント ID */
	ULONG length; /* セグメントの長さ */
	BYTE subID; /* 言語指定の時、連鎖の数 */
	BYTE attribute; /* 言語指定の時、言語の種類 */
	TCODE code;
	/* 書き込みバッファ */
	BYTE buf[WRITEBUFSIZE];
	int BufIndex;
	LONG BufStartOffset;
} FILECONTEXT;

#define NONE -1

#define TS_FIXED 1 /* 固定長セグメント */
#define TS_SPECIAL 2 /* TRON 仕様特殊コード */
#define TS_LANGUAGE 3 /* 言語指定コード */

extern FILECONTEXT* StartFile();
extern BOOL EndFile();
extern BOOL ReadLinkRecode();
extern BOOL NextSegment();
extern BOOL ReadSubID();
extern void SkipData();
extern BOOL SeekFile();
extern int ReadData();
extern BOOL WriteData();
extern BOOL WriteSegmentHead();
