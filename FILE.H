#define WRITEBUFSIZE 1024
/* �t�@�C���ǂݍ��݂̏�Ԃ�\���\���� */
typedef struct{
	WORD fd;
	LONG TADRecode;
	LONG LinkRecode;
	LONG offset; /* ���݂̃I�t�Z�b�g */
				 /* �ϒ��Z�O�����g�̎��F�f�[�^�{�̂̍ŏ��i�f�[�^���̌�j */
				 /* �Œ蒷�Z�O�����g�̎��F���̕����̂Ƃ��� */
	/* �������ǂݍ��ݗp�f�[�^ */
	LONG SegOffset; /* �Z�O�����g�̍ŏ��̃I�t�Z�b�g */
	LONG NextOffset; /* ���̃Z�O�����g�̃I�t�Z�b�g */
	BYTE ID; /* �Z�O�����g ID */
	ULONG length; /* �Z�O�����g�̒��� */
	BYTE subID; /* ����w��̎��A�A���̐� */
	BYTE attribute; /* ����w��̎��A����̎�� */
	TCODE code;
	/* �������݃o�b�t�@ */
	BYTE buf[WRITEBUFSIZE];
	int BufIndex;
	LONG BufStartOffset;
} FILECONTEXT;

#define NONE -1

#define TS_FIXED 1 /* �Œ蒷�Z�O�����g */
#define TS_SPECIAL 2 /* TRON �d�l����R�[�h */
#define TS_LANGUAGE 3 /* ����w��R�[�h */

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