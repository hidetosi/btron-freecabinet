#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "file.h"
#include "databox.h"

extern void ErrEnd(); /* view.c */

/* #define FORMAL_TAD */
#define SEMI_TAD

int ReadData(afcFile,apData,aiSize)
FILECONTEXT* afcFile;
BYTE* apData;
int aiSize;
{
	LONG lRestSize;

	rea_rec(afcFile->fd,afcFile->offset,apData,(LONG)aiSize,
			&lRestSize,(UWORD*)NULL);
	afcFile->offset+=aiSize;
	if(lRestSize<aiSize) /* ファイルの最後に達したら */
	{ /* 次の主TAD レコードを探す */
		LONG lNextTAD;

		if(fnd_rec(afcFile->fd,F_NFWD,0x00000002L,0,&lNextTAD)!=1)
			return lRestSize;
		afcFile->offset=0;
		afcFile->TADRecode=lNextTAD;
		rea_rec(afcFile->fd,afcFile->offset,apData+lRestSize,
				(LONG)(aiSize-lRestSize),(LONG*)NULL,(UWORD*)NULL);
		afcFile->offset+=aiSize-lRestSize;
		return aiSize;
	}else
		return aiSize;
}


BOOL FlushBuf(afcFile)
FILECONTEXT* afcFile;
{
	if(afcFile->BufIndex==0)
		return TRUE;

	see_rec(afcFile->fd,afcFile->TADRecode,1,(LPTR)NULL);
	if(wri_rec(afcFile->fd,afcFile->BufStartOffset,afcFile->buf,
		(LONG)afcFile->BufIndex,NULL,(UWORD*)NULL,0)<0)
	{
		ErrPanel(ID_ST_NWDATA,FALSE);
		return FALSE;
	}
	afcFile->BufStartOffset+=afcFile->BufIndex;
	afcFile->BufIndex=0;

	return TRUE;
}


BOOL WriteData(afcFile,apData,aiSize)
FILECONTEXT* afcFile;
BYTE* apData;
int aiSize;
{
	int i;

	if(afcFile->BufIndex==0)
		afcFile->BufStartOffset=afcFile->offset;
	for(i=0; i<aiSize; i++)
	{
		afcFile->buf[afcFile->BufIndex]=apData[i];
		afcFile->BufIndex++;
		afcFile->offset++;
		if(afcFile->BufIndex>=WRITEBUFSIZE)
		{
			if(!FlushBuf(afcFile))
				return FALSE;
		}
	}
	return TRUE;
}


/* TAD 読み書きの準備 */
FILECONTEXT* StartFile(awFileDscr,alRecode)
WORD awFileDscr;
LONG alRecode; /* 主TAD レコード */
{
	FILECONTEXT* fcNew;

	if(get_lmb(&fcNew,(LONG)sizeof(FILECONTEXT),NOCLR)<0)
	{
		ErrPanel(0,FALSE); /* メモリ割り当てエラー */
		return NULL;
	}
	fcNew->fd=awFileDscr;
	fcNew->offset=0;
	fcNew->SegOffset=0;
	fcNew->NextOffset=0;
	fcNew->length=0;
	fcNew->TADRecode=alRecode;
	fcNew->LinkRecode=0;
	fcNew->BufIndex=0;

	return fcNew;
}


BOOL EndFile(afcFile)
FILECONTEXT* afcFile;
{
	if(!FlushBuf(afcFile))
	{
		rel_lmb(afcFile);
		return FALSE;
	}
	rel_lmb(afcFile);
	return TRUE;
}


void SkipData(afcFile,aiSize)
FILECONTEXT* afcFile;
int aiSize;
{
	afcFile->offset+=aiSize;
}

BOOL SeekFile(afcFile,alOffset)
FILECONTEXT* afcFile;
LONG alOffset;
{
	if(!FlushBuf(afcFile))
		return FALSE;
	afcFile->offset=alOffset;
	return TRUE;
}


/* 次のリンクレコードを探して読み込む */
BOOL ReadLinkRecode(afcFile,apLink)
FILECONTEXT* afcFile;
VLINK* apLink;
{
	LONG lLinkRecode;

	see_rec(afcFile->fd,afcFile->LinkRecode,1,(LPTR)NULL);
	if(fnd_rec(afcFile->fd,F_FWD,1L,0,(LPTR)(&lLinkRecode))<0)
	{
		static BOOL isCameFirst=TRUE; /* エラーパネルを出すのは1回だけ */

		if(isCameFirst)
		{
			ErrPanel(ID_ST_NFNDLINK,FALSE);
			isCameFirst=FALSE;
		}
		return FALSE;
	}
	afcFile->LinkRecode=lLinkRecode;

	if(rea_rec(afcFile->fd,0L,apLink,(LONG)sizeof(VLINK),NULL,NULL) < 0)
	{
		static BOOL isCameFirst=TRUE; /* エラーパネルを出すのは1回だけ */

		if(isCameFirst)
		{
			ErrPanel(ID_ST_NREADLINK,FALSE);
			isCameFirst=FALSE;
		}
		return FALSE;
	}

	afcFile->LinkRecode++;

	return TRUE;
}

/* 次のセグメントに移動し、ID,長さを調べる
   文字かどうかも調べる */
BOOL NextSegment(afcFile,isDoubleByte)
FILECONTEXT* afcFile;
BOOL isDoubleByte;
{
	UWORD IDCheck;
	LONG lRecode;

	afcFile->offset=afcFile->NextOffset; /* オフセットを進める */
	afcFile->SegOffset=afcFile->offset;

	/* レコード番号が異なっていたら元に戻す */
	see_rec(afcFile->fd,0L,0,&lRecode);
	if(lRecode!=afcFile->TADRecode)
		see_rec(afcFile->fd,afcFile->TADRecode,1,NULL);

#ifdef SEMI_TAD
	if(ReadData(afcFile,&IDCheck,2)<2) /* 読み込めなかったら */
		return FALSE;

	if( (IDCheck&0xff00)>>8 == 0x00ff )
	{
		UWORD ID;

		ID=IDCheck&0x00ff;
		if(0x21<=ID && ID<=0x7f) /* TRON 仕様特殊コードの時 */
		{
			afcFile->offset-=2;
			afcFile->ID=TS_SPECIAL;
			afcFile->length=2;
			afcFile->code=ID;
		}else{ /* 可変長セグメントなら */
			UWORD LengthCheck;

			afcFile->ID=ID;
			if(ReadData(afcFile,&LengthCheck,2)<2)
				return FALSE;
			if(LengthCheck!=0xffff) /* 通常セグメントなら */
				afcFile->length=(ULONG)LengthCheck;
			else{ /* ラージセグメントなら */
				if(ReadData(afcFile,&(afcFile->length),4)<4)
					return FALSE;
			}
		}
	}else if(IDCheck&0xff00==0xfe00) /* 言語指定コードなら */
	{
		BOOL flag;
		int count;
		BYTE lang;

		flag=TRUE;
		for(count=0; flag; count++);
		{
			if(ReadData(afcFile,&lang,1)<0)
				return FALSE;
			if(lang!=0xfe) /* 連鎖でないなら */
				flag=FALSE;
		}
		afcFile->offset-=count+2;
		afcFile->ID=TS_LANGUAGE;
		afcFile->subID=count;
		afcFile->attribute=lang;
		afcFile->length=0;
	}else{ /* 固定長セグメント（文字コード）なら */
		afcFile->offset-=2;
		afcFile->ID=TS_FIXED;
		afcFile->length=2;
		afcFile->code=IDCheck;
	}
	afcFile->NextOffset=afcFile->offset+afcFile->length;

#endif
#ifdef FORMAL_TAD
#endif

	return TRUE;
}


BOOL WriteSegmentHead(afcFile,awID,alLength,aiSubID,aiAttrib)
FILECONTEXT* afcFile;
UWORD awID;
LONG alLength;
int aiSubID;
int aiAttrib;
{
	UWORD wData;
	WORD wErr;

	wErr=0;
	awID=awID| 0xff00;
	wErr+=!WriteData(afcFile,&awID,sizeof(UWORD));
	if(alLength>0xffff)
	{
		wData=0xffff;
		wErr+=!WriteData(afcFile,&wData,sizeof(UWORD));
		wErr+=!WriteData(afcFile,&alLength,sizeof(LONG));
	}else{
		wData=(UWORD)alLength;
		wErr+=!WriteData(afcFile,&wData,sizeof(UWORD));
	}
	if(aiSubID!=NONE)
	{
		BYTE bData;

		bData=aiAttrib;
		wErr+=!WriteData(afcFile,&bData,sizeof(BYTE));
		bData=aiSubID;
		wErr+=!WriteData(afcFile,&bData,sizeof(BYTE));
	}
	if(wErr!=0)
		return FALSE;
	return TRUE;
}


BOOL ReadSubID(afcFile)
FILECONTEXT* afcFile;
{
#ifdef SEMI_TAD
	if(ReadData(afcFile,&(afcFile->attribute),1)<1)
		return FALSE;
	if(ReadData(afcFile,&(afcFile->subID),1)<1)
		return FALSE;
#endif
#ifdef FORMAL_TAD
	if(ReadData(afcFile,&(afcFile->subID),1)<1)
		return FALSE;
	if(ReadData(afcFile,&(afcFile->attribute),1)<1)
		return FALSE;
#endif
	return TRUE;
}
