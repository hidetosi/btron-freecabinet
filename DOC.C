#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "file.h"
#include "doc.h"
#include "databox.h"

#define COORDINATE_MIN 5

#define NOSAVE 0 /* セーブの必要なし */
#define CHANGEORIGINALDATA	0x0001 /* 固有データが変更された */
#define CHANGESEGMENT	0x0002 /* 仮身セグメント(固有データ以外)が変更された */
#define CHANGELINK	0x0004 /* リンクレコードが変更された(続柄の変更) */
#define NEWDATA	0x0008 /* 最後に追加されたデータがある */
#define SAVETAD 0x0010 /* TAD 主レコードだけセーブ */
#define ALLSAVE	0x0020 /* 全体のセーブが必要 */
#define ILLEGALTAD	0x0040 /* 想定している TAD とは異なる TAD だった。
						セーブするなら TAD 全体をセーブしなければならない。 */
#define ILLEGALFILE	0x0080 /* 変更があった場合、ファイル全体を
												セーブしなくてはならない */
#define SILENTSAVE	0x0100 /* こっそりセーブ */

/* 仮身1つのデータ構造 */
typedef struct tag_VOBJDATA{
	struct tag_VOBJDATA* next;
	struct tag_VOBJDATA* previous;
	WORD idVirtualObj; /* 仮身 ID */
	BOOL isFusen; /* TRUE=付箋 FALSE=仮身 */
	RECT rctView; /* 表示位置 */
	BOOL isSelect; /* 選択されているかどうか */
#define VDST_NORMAL	0
#define VDST_FIXED	1
#define VDST_BACKGROUND	2
	int iState; /* 固定化・背景化 */

	/* 部分保存のためのデータ */
	int iModified; /* 変更状況 */
	LONG lOffset; /* ファイル内での仮身セグメントのオフセット */
	UWORD wSize; /* 仮身セグメントのサイズ */
	LONG lRecode; /* リンクのレコード番号 */
} VOBJDATA;


typedef struct{
	VOBJDATA* vdFirst; /* 最初の仮身 */
	VOBJDATA* vdLast; /* 最後の仮身 */
	RECT rctView; /* すべての仮身を囲む領域 */
	WORD idVirtualObj; /* このファイルの仮身ID */
	WORD wFileDscr; /* ファイルディスクリプタ */
	BOOL isReadOnly; /* リードオンリーかどうか */
	WORD wHorizontalReso; /* 水平解像度 */
	WORD wVerticalReso; /* 垂直解像度 */
	int iSelectedNum; /* 選択された仮身の数 */
	int iVobjNum; /* 仮身の数 */
	int iFusenNum; /* 付箋の数 */
	int iModified; /* 変更状況 */
	LONG lLastOffset; /* 追加書き込みの時に書き込み始めるオフセット */
	VOBJDATA* vdLastInDisk; /* 読み込み時の最後の仮身(追加書き込み用) */
	VLINK* lnkInDisk; /* ディスクに入っていたリンク */
	int iLinkNum; /* ディスクに入っていたリンクの数 */
} DOCDATA;

DOCDATA document={NULL,NULL,{0,0,0,0},0,0,FALSE,0,0,0,0,0,NOSAVE,0L,NULL,NULL,0};


/* データ全体の RECT の大きさより外に新たなデータが出来たら
   document.rctView を大きくする。
   （大きくするだけ。小さくするには他の関数を使う。 */
void CheckViewSize(arctNew)
RECT arctNew;
{
	if(arctNew.c.left<document.rctView.c.left+COORDINATE_MIN)
		document.rctView.c.left=arctNew.c.left-COORDINATE_MIN;
	if(arctNew.c.top<document.rctView.c.top+COORDINATE_MIN)
		document.rctView.c.top=arctNew.c.top-COORDINATE_MIN;
	if(arctNew.c.right>document.rctView.c.right-COORDINATE_MIN)
		document.rctView.c.right=arctNew.c.right+COORDINATE_MIN;
	if(arctNew.c.bottom>document.rctView.c.bottom-COORDINATE_MIN)
		document.rctView.c.bottom=arctNew.c.bottom+COORDINATE_MIN;
}


/* 仮身のデータ(VOBJDATA)を追加する */
void AddVobjData(avdData,aisLast)
VOBJDATA* avdData;
BOOL aisLast; /* TRUE=最後に追加する FALSE=最初に追加する */
{
	if(document.vdFirst!=NULL) /* 2つ目以降なら */
	{
		if(aisLast)
		{ /* 最後に追加する */
			avdData->previous=document.vdLast;
			avdData->next=NULL;
			document.vdLast->next=avdData;
			document.vdLast=avdData;
		}else{ /* 最初に追加する */
			avdData->next=document.vdFirst;
			avdData->previous=NULL;
			document.vdFirst->previous=avdData;
			document.vdFirst=avdData;
		}
	}else{ /* 最初の項目なら */
		document.vdFirst=avdData;
		document.vdLast=avdData;
		avdData->next=NULL;
		avdData->previous=NULL;
	}

	CheckViewSize(avdData->rctView);

	if(avdData->isFusen)
		document.iFusenNum++;
	else
		document.iVobjNum++;
}

BOOL AddVobjByID(aidVobj)
WORD aidVobj;
{
	VOBJDATA* pvdNew;
	VOBJSEG vsVSeg;
	VFPTR vfPoint;

	vfPoint.vobj=&vsVSeg;
	oget_vob(aidVobj,(VLINKPTR)NULL,vfPoint,sizeof(VOBJSEG),(WORD*)NULL);

	if(get_lmb(&pvdNew,(LONG)sizeof(VOBJDATA),NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		return FALSE;
	}
	pvdNew->idVirtualObj=aidVobj;
	pvdNew->rctView=vfPoint.vobj->view;
	pvdNew->isSelect=FALSE;
	pvdNew->iModified=NEWDATA;
	pvdNew->lOffset=0;
	pvdNew->wSize=0;
	pvdNew->iState=VDST_NORMAL;
	if(oget_vob(aidVobj,NULL,NULL,0,NULL)==1) /* 付箋の時 */
		pvdNew->isFusen=TRUE;
	else
		pvdNew->isFusen=FALSE;

	AddVobjData(pvdNew,TRUE);

	document.iModified|=NEWDATA;

	return TRUE;
}


/* --------------------ファイルロード関連-------------------- */


BOOL FileOpen(aidVirtual,alnkOpen)
WORD aidVirtual;
LINK alnkOpen;
{
	WORD wFileDscr;

	document.idVirtualObj=aidVirtual;

	if(chk_fil(&alnkOpen,F_READ|F_EXCUTE,NULL)<0)
	{
		ErrPanel(ID_ST_NACSES,FALSE);
		return FALSE;
	}
	if(chk_fil(&alnkOpen,F_WRITE,NULL)<0)
	{ /* リードオンリー */
		wFileDscr=opn_fil(&alnkOpen,F_READ|F_WEXCL,NULL);
		if(wFileDscr<0)
		{
			ErrPanel(ID_ST_NOPENFILE,FALSE);
			return FALSE;
		}
		ErrPanel(ID_ST_RDONLY,TRUE);
		document.isReadOnly=TRUE;
	}else{
		/* ファイルのオープン */
		wFileDscr=opn_fil(&alnkOpen,F_UPDATE|F_WEXCL,NULL);
		if(wFileDscr<0)
		{
			switch(wFileDscr)
			{
			case E_BUSY: /* 他のアプリですでに開かれている */
				wFileDscr=opn_fil(&alnkOpen,F_READ,NULL);
				if(wFileDscr<0)
				{
					ErrPanel(ID_ST_NOPNBUSY,TRUE);
					return FALSE;
				}
				ErrPanel(ID_ST_OPNBUSY,TRUE);
				document.isReadOnly=TRUE;
				break;
			case E_ACCES: /* アクセス権限がない */
				ErrPanel(ID_ST_NACSES,FALSE);
				return FALSE;
			default:
				ErrPanel(ID_ST_NOPENFILE,FALSE);
				return FALSE;
			}
		}
	}
	document.wFileDscr=wFileDscr;
	return TRUE;
}

BOOL ReadVobj(afcFile,aidWnd,aiLinkIndex,aiState)
FILECONTEXT* afcFile;
WORD aidWnd;
int aiLinkIndex; /* -1 なら付箋 */
int aiState;
{
	VOBJDATA* pvdItem;
	VFPTR vfItem;
	VLINK* plnkLink;
	WORD idVobj;
	int iSize;
	LONG lOffset;

	lOffset=afcFile->offset;

	/* 仮身セグメントの読み込み(あまり良くない方法) */ 
	iSize=afcFile->length;
	if(get_lmb(&(vfItem.vobj),(LONG)iSize,NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		return FALSE;
	}
	ReadData(afcFile,vfItem.vobj,iSize);

	/* リンクレコードの読み込み */
	if( aiLinkIndex==-1)
		plnkLink=NULL;
	else{
		if(!ReadLinkRecode(afcFile,&(document.lnkInDisk[aiLinkIndex])))
		{
			rel_lmb(vfItem.vobj);
			return FALSE;
		}
		plnkLink=&(document.lnkInDisk[aiLinkIndex]);
	}

	/* 仮身の登録 */
	if(aidWnd<0) /* 開いた仮身の時 */
		idVobj=oreg_vob(plnkLink,vfItem,
									-aidWnd,V_DISPALL|V_NOFRAME);
	else /* 仮身のオープンの時 */
		idVobj=oreg_vob(plnkLink,vfItem,aidWnd,V_DISPALL);
	if(idVobj<0) /* 登録できなかったら */
	{
		ErrPanel(ID_ST_NREGVOBJ,FALSE);
		rel_lmb(vfItem);
		return FALSE;
	}

	/* 項目へメモリー割り当て */
	if(get_lmb(&pvdItem,(LONG)sizeof(VOBJDATA),NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		rel_lmb(vfItem);
		return FALSE;
	}

	/* 項目の要素をセット */
	pvdItem->rctView=vfItem.vobj->view; /* 仮身も付箋もここだけ共通 */
	pvdItem->idVirtualObj=idVobj;
	pvdItem->isSelect=FALSE;
	pvdItem->iModified=NOSAVE;
	pvdItem->lOffset=lOffset;
	pvdItem->wSize=iSize;
	pvdItem->iState=aiState;
	if(aiLinkIndex==-1)
	{
		pvdItem->isFusen=TRUE;
		pvdItem->lRecode=0;
	}else{
		pvdItem->isFusen=FALSE;
		pvdItem->lRecode=afcFile->LinkRecode-1;
	}

	AddVobjData(pvdItem,TRUE);
	document.vdLastInDisk=pvdItem;

	rel_lmb(vfItem);

	return TRUE;
}

int ReadFigAplSeg(afcFile)
FILECONTEXT* afcFile;
{
	UWORD wApl[3];

	if(!ReadSubID(afcFile))
		return -1;
	if(ReadData(afcFile,wApl,6)<6)
		return -1;
	if( !(wApl[0]==0x8000 && wApl[1]==0x0000 && wApl[2]==0x8000) )
		return -2;
	if(afcFile->subID==VDST_NORMAL)
		return VDST_NORMAL;
	if(afcFile->subID==VDST_FIXED)
		return VDST_FIXED;
	if(afcFile->subID==VDST_BACKGROUND)
		return VDST_BACKGROUND;
	return -2;
}

/* 主 TAD レコードに対応してない仮身レコードと機能付箋レコードを読み込む */
BOOL ReadNotInTADRecord(afcFile,aidWnd,aiLinkIndex)
FILECONTEXT* afcFile;
WORD aidWnd;
int aiLinkIndex;
{
	BOOL isAppend;

	isAppend=FALSE;
	/* 仮身レコードを読み込む */
	see_rec(afcFile->fd,afcFile->LinkRecode,1,(LONG*)NULL);
	while(fnd_rec(afcFile->fd,F_FWD,0x00000001L,0,(LONG*)NULL)==0)
	{
		VOBJDATA* pvdItem;
		VFPTR vfItem;
		VLINK* plnkLink;
		WORD idVobj;

		/* 仮身セグメントを適当に作成する */ 
		if(get_lmb(&(vfItem.vobj),(LONG)sizeof(VOBJSEG),NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			return FALSE;
		}
		vfItem.vobj->view.c.left=COORDINATE_MIN;
		vfItem.vobj->view.c.right=150+COORDINATE_MIN;
		vfItem.vobj->view.c.top=document.rctView.c.bottom+5;
		vfItem.vobj->view.c.bottom=document.rctView.c.bottom+5+22;
		vfItem.vobj->height=0;
		vfItem.vobj->chsz=0;
		vfItem.vobj->frcol=15;
		vfItem.vobj->tbcol=0x10DFDFDF;
		vfItem.vobj->chcol=15;
		vfItem.vobj->bgcol=0;
		vfItem.vobj->dlen=0;

		/* リンクレコードの読み込み */
		if(rea_rec(afcFile->fd,0L,&(document.lnkInDisk[aiLinkIndex]),
					(LONG)sizeof(VLINK),NULL,NULL) < 0)
		{
			rel_lmb(vfItem.vobj);
			return FALSE;
		}
		plnkLink=&(document.lnkInDisk[aiLinkIndex]);
		see_rec(afcFile->fd,1L,0,(LONG*)NULL); /* 次のレコードへ */

		/* 仮身の登録 */
		if(aidWnd<0) /* 開いた仮身の時 */
			idVobj=oreg_vob(plnkLink,vfItem,
										-aidWnd,V_DISPALL|V_NOFRAME);
		else /* 仮身のオープンの時 */
			idVobj=oreg_vob(plnkLink,vfItem,aidWnd,V_DISPALL);
		if(idVobj<0) /* 登録できなかったら */
		{
			ErrPanel(ID_ST_NREGVOBJ,FALSE);
			rel_lmb(vfItem);
			return FALSE;
		}

		/* 項目へメモリー割り当て */
		if(get_lmb(&pvdItem,(LONG)sizeof(VOBJDATA),NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			rel_lmb(vfItem);
			return FALSE;
		}

		/* 項目の要素をセット */
		pvdItem->rctView=vfItem.vobj->view;
		pvdItem->idVirtualObj=idVobj;
		pvdItem->isSelect=FALSE;
		pvdItem->iModified=NEWDATA;
		pvdItem->lOffset=0;
		pvdItem->wSize=0;
		pvdItem->iState=VDST_NORMAL;
		pvdItem->isFusen=FALSE;
		pvdItem->lRecode=0;

		AddVobjData(pvdItem,TRUE);

		aiLinkIndex++;
		rel_lmb(vfItem);
		isAppend=TRUE;
	}

	if(isAppend)
	{
		document.iModified|=SAVETAD;
		document.iModified&=~SILENTSAVE;
	}


	/* 機能付箋レコードを読み込む */
	isAppend=FALSE;
	see_rec(afcFile->fd,0L,1,(LONG*)NULL);
	while(fnd_rec(afcFile->fd,F_FWD,128L,0,(LONG*)NULL)==7)
	{
		VOBJDATA* pvdItem;
		VFPTR vfItem;
		WORD idVobj;
		LONG lSize;

		rea_rec(afcFile->fd,0L,NULL,0L,&lSize,NULL); /* レコードサイズを得る */

		/* メモリ割り当て */ 
		if(get_lmb(&(vfItem.fsn),lSize,NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			return FALSE;
		}

		/* 機能付箋レコードの読み込み */
		if(rea_rec(afcFile->fd,0L,vfItem.fsn,lSize,NULL,NULL) < 0)
		{
			rel_lmb(vfItem.vobj);
			return FALSE;
		}

		see_rec(afcFile->fd,1L,0,(LONG*)NULL); /* 次のレコードへ */

		/* 付箋の登録 */
		if(aidWnd<0) /* 開いた仮身の時 */
			idVobj=oreg_vob(NULL,vfItem,-aidWnd,V_DISPALL|V_NOFRAME);
		else /* 仮身のオープンの時 */
			idVobj=oreg_vob(NULL,vfItem,aidWnd,V_DISPALL);
		if(idVobj<0) /* 登録できなかったら */
		{
			ErrPanel(ID_ST_NREGVOBJ,FALSE);
			rel_lmb(vfItem);
			return FALSE;
		}

		/* 項目へメモリー割り当て */
		if(get_lmb(&pvdItem,(LONG)sizeof(VOBJDATA),NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			rel_lmb(vfItem);
			return FALSE;
		}

		/* 項目の要素をセット */
		pvdItem->rctView=vfItem.vobj->view;
		pvdItem->idVirtualObj=idVobj;
		pvdItem->isSelect=FALSE;
		pvdItem->iModified=NEWDATA;
		pvdItem->lOffset=0;
		pvdItem->wSize=0;
		pvdItem->iState=VDST_NORMAL;
		pvdItem->isFusen=TRUE;
		pvdItem->lRecode=0;

		AddVobjData(pvdItem,TRUE);

		rel_lmb(vfItem);
		isAppend=TRUE;
	}

	if(isAppend)
	{
		document.iModified|=ALLSAVE;
		document.iModified&=~SILENTSAVE;
		document.iModified|=ILLEGALFILE;
	}

	return TRUE;
}

BOOL ReadFile(aidWnd)
WORD aidWnd;
{
	LONG lTADRecode;
	FILECONTEXT* fcFile;
	BOOL isStartSegCame;
	BOOL isText;
	BOOL isRightTAD; /* 想定したとおりの TAD かどうか */
	BOOL isErrLink;
	LONG lLastOffset; /* 追加書き込みの時に書き込み始めるオフセット */
	F_STATE fsState;
	int iLinkIndex;
	int iState;

	ofl_sts(document.wFileDscr,(TPTR)NULL,&fsState,(F_LOCATE*)NULL);
									/* 含まれているリンクレコードの数を得る */
	document.iLinkNum=fsState.f_nlink;
	if(document.iLinkNum!=0)
	{
		if(get_lmb(&(document.lnkInDisk),(LONG)(fsState.f_nlink*sizeof(VLINK)),
																	NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			return FALSE;
		}
	}else
		document.lnkInDisk=NULL;

	/* TAD 主レコードを探す */
	if(fnd_rec(document.wFileDscr,F_TOPEND,0x00000002L,0,&lTADRecode)<0)
	{ /* 見つからなかったら */
		document.wHorizontalReso=-120;
		document.wVerticalReso=-120;
		return TRUE;
	}

	fcFile=StartFile(document.wFileDscr,lTADRecode);
	if(fcFile==NULL)
		return FALSE;

	iLinkIndex=0;
	isStartSegCame=FALSE;
	isRightTAD=TRUE;
	isErrLink=FALSE;
	iState=VDST_NORMAL;
	/* セグメントを1つずつ読んでいく */
	while(NextSegment(fcFile,TRUE))
	{
/*b_printf("ID=%d\n",fcFile->ID);*/
		switch(fcFile->ID)
		{
		case TS_FIG: /* 図形開始セグメントの時 */
			if(isStartSegCame)
			{
				isRightTAD=FALSE;
				break;
			}
			isStartSegCame=TRUE;
			isText=FALSE;
			SkipData(fcFile,16);
			if(ReadData(fcFile,&(document.wHorizontalReso),sizeof(WORD))<0)
			{
				isRightTAD=FALSE;
				break;
			}
			if(ReadData(fcFile,&(document.wVerticalReso),sizeof(WORD))<0)
				isRightTAD=FALSE;
			break;

		case TS_TEXT: /* 文章開始セグメントの時 */
			isRightTAD=FALSE;
			if(isStartSegCame)
				break;
			isStartSegCame=TRUE;
			isText=TRUE;
			SkipData(fcFile,16);
			if(ReadData(fcFile,&(document.wHorizontalReso),sizeof(WORD))<0)
			{
				isRightTAD=FALSE;
				break;
			}
			if(ReadData(fcFile,&(document.wVerticalReso),sizeof(WORD))<0)
				isRightTAD=FALSE;
			break;

		case TS_FIGEND: /* 図形終了セグメントの時 */
			if(isStartSegCame && !isText)
				lLastOffset=fcFile->SegOffset;
			else
				isRightTAD=FALSE;
			break;

		case TS_TEXTEND: /* 文章終了セグメントの時 */
			isRightTAD=FALSE;
			if(isStartSegCame && isText)
				lLastOffset=fcFile->SegOffset;
			break;

		case TS_VOBJ: /* 仮身セグメントの時 */
			if(!isStartSegCame)
				break;
			lLastOffset=fcFile->NextOffset;
			if(!ReadVobj(fcFile,aidWnd,iLinkIndex,iState))
			{
				isErrLink=TRUE;
				isRightTAD=FALSE;
				break;
			}
			iLinkIndex++;
			break;

		case TS_FFUSEN: /* 機能付箋の時 */
			if(!isStartSegCame)
				break;
			lLastOffset=fcFile->NextOffset;
			if(!ReadVobj(fcFile,aidWnd,-1,iState))
			{
				isRightTAD=FALSE;
				break;
			}
			break;

		case TS_FAPPL: /* 図形アプリケーション指定付箋 */
			{
				int iNewState;

				iNewState=ReadFigAplSeg(fcFile);
				if(iNewState==-1) /* ファイルの最後までいった */
				{
					isRightTAD=FALSE;
					break;
				}
				if(iNewState==-2) /* 知らない内容だった */
					break;
				iState=iNewState;
				break;
			}

		case TS_INFO:
			break;
		default:
			isRightTAD=FALSE;
		}
	}
	document.lLastOffset=lLastOffset;

	if(lTADRecode!=(fcFile->TADRecode))
							/* 複数の TAD レコードに分かれて入っていたら */
		isRightTAD=FALSE;

	if(!isRightTAD)
	{
		ErrPanel(ID_ST_ILLTAD,TRUE);
		document.iModified=ILLEGALTAD;
	}else
		document.iModified=NOSAVE;
	if(isErrLink)
	{
		ErrPanel(ID_ST_ILLTAD,TRUE);
		document.iModified|=ILLEGALFILE;
	}

	/* 主 TAD レコードに対応してない仮身レコードと機能付箋レコードを読み込む */
	ReadNotInTADRecord(fcFile,aidWnd,iLinkIndex);

	if(document.iVobjNum+document.iFusenNum==0)
		document.iModified=ILLEGALFILE;

	EndFile(fcFile);

	return TRUE;
}


/* --------------------ファイルセーブ関連-------------------- */


BOOL WriteInfoSeg(afcFile)
FILECONTEXT* afcFile;
{
	UWORD wData;

	WriteSegmentHead(afcFile,TS_INFO,6L,NONE,NONE);
	wData=0; /* TAD規格のバージョン番号の指定の SubID */
	WriteData(afcFile,&wData,sizeof(UWORD));
	wData=2; /* 項目の長さ */
	WriteData(afcFile,&wData,sizeof(UWORD));
	wData=0x0130; /* TAD規格のバージョン番号 */
	WriteData(afcFile,&wData,sizeof(UWORD));

	return TRUE;
}

BOOL WriteFigStartSeg(afcFile)
FILECONTEXT* afcFile;
{
	WORD wHorizontalReso,wVerticalReso;
	LONG lNullData;

	WriteSegmentHead(afcFile,TS_FIG,24L,NONE,NONE);
	WriteData(afcFile,&(document.rctView.c.left),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.right),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.top),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.bottom),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.left),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.right),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.top),sizeof(WORD));
	WriteData(afcFile,&(document.rctView.c.bottom),sizeof(WORD));

	WriteData(afcFile,&(document.wHorizontalReso),sizeof(WORD));
	WriteData(afcFile,&(document.wVerticalReso),sizeof(WORD));

	lNullData=0;
	WriteData(afcFile,&(lNullData),sizeof(LONG));

	return TRUE;
}

BOOL WriteFigEndSeg(afcFile)
FILECONTEXT* afcFile;
{
	return WriteSegmentHead(afcFile,TS_FIGEND,0L,NONE,NONE);
}

BOOL WriteFigAplSegment(afcFile,aiState)
FILECONTEXT* afcFile;
int aiState;
{
	UWORD wApl[3];

	if(!WriteSegmentHead(afcFile,TS_FAPPL,8L,aiState,0x0001))
	{
		ErrPanel(ID_ST_NWDATA,FALSE);
		return FALSE;
	}
	wApl[0]=0x8000;
	wApl[1]=0x0000;
	wApl[2]=0x8000;
	if(!WriteData(afcFile,wApl,6))
	{
		ErrPanel(ID_ST_NWDATA,FALSE);
		return FALSE;
	}

	return TRUE;
}

BOOL WriteVirtualObjSeg(afcFile,avdCurrent,apiState)
FILECONTEXT* afcFile;
VOBJDATA* avdCurrent;
int* apiState;
{
	UWORD wSize;
	VFPTR vfData;

	/* 固定化・背景化の処理 */
	if(*apiState!=avdCurrent->iState)
	{
		WriteFigAplSegment(afcFile,avdCurrent->iState);
		*apiState=avdCurrent->iState;
	}

	/* 仮身セグメントの取得 */
	oget_vob(avdCurrent->idVirtualObj,(VLINKPTR*)NULL,(VFPTR*)NULL,0,&wSize);
	if(get_lmb(&vfData,(LONG)wSize,NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		return FALSE;
	}
	oget_vob(avdCurrent->idVirtualObj,(VLINKPTR*)NULL,
				vfData,wSize,(UWORD*)NULL);

	if(!avdCurrent->isFusen)
	{
		if(!WriteSegmentHead(afcFile,TS_VOBJ,wSize,NONE,NONE))
		{
			ErrPanel(ID_ST_NWDATA,FALSE);
			return FALSE;
		}
	}else{
		if(!WriteSegmentHead(afcFile,TS_FFUSEN,wSize,NONE,NONE))
		{
			ErrPanel(ID_ST_NWDATA,FALSE);
			return FALSE;
		}
	}
	avdCurrent->lOffset=afcFile->offset;
	if(!WriteData(afcFile,vfData.vobj,wSize))
	{
		ErrPanel(ID_ST_NWDATA,FALSE);
		return FALSE;
	}

	avdCurrent->wSize=wSize;
	avdCurrent->iModified=NOSAVE;

	rel_lmb(vfData);

	return TRUE;
}


BOOL WriteLinkRecode(awFileDscr,aidVirtual)
WORD awFileDscr;
WORD aidVirtual;
{
	VOBJDATA* vdAppend;
	VLINK* lnkNew;
	int i;

	if(document.iVobjNum!=0)
	{
		if(get_lmb(&lnkNew,(LONG)(document.iVobjNum*sizeof(VLINK)),NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			return FALSE;
		}
	}

	/* 古いリンクレコードの削除 */
	see_rec(awFileDscr,0L,1,(LONG*)NULL); /* 最初のレコードに移動 */
	while(fnd_rec(awFileDscr,F_FWD,0x00000001L,0,(LONG*)NULL) == 0)
	{
		if(del_rec(awFileDscr)<0) /* リンクレコードを削除 */
		{
			ErrPanel(ID_ST_NDELLINK,FALSE);
			return FALSE;
		}
	}

	/* リンクレコードの追加 */
	vdAppend=document.vdFirst;
	for(i=0; vdAppend!=NULL; vdAppend=vdAppend->next)
	{
		if(vdAppend->isFusen)
			continue;
		ocnv_vob(aidVirtual,vdAppend->idVirtualObj,&(lnkNew[i]));
		/* 最後のレコードにリンクレコードを追加する */
		if((apd_rec(awFileDscr,&(lnkNew[i]),(LONG)sizeof(VLINK),0,0,0))<0)
		{
			ErrPanel(ID_ST_NWLINK,FALSE);
			return FALSE;
		}
		see_rec(awFileDscr,0L,0,&(vdAppend->lRecode));
												/* 現在のレコード番号を得る */
		i++;
	}

	/* 参照数が 0 になった実身を削除する */
	for(i=0; i<document.iLinkNum; i++)
	{
		if(fil_sts(&(document.lnkInDisk[i]),(TPTR)NULL,
					(F_STATE*)NULL,(F_LOCATE*)NULL)==0) /* 参照数が 0 なら */
			odel_obj(&(document.lnkInDisk[i]),0); /* ファイルの削除 */
	}

	rel_lmb(document.lnkInDisk);
	document.lnkInDisk=lnkNew;
	document.iLinkNum=document.iVobjNum;

	return TRUE;
}

BOOL WriteTAD(afcFile)
FILECONTEXT* afcFile;
{
	VOBJDATA* vdCurrent;
	int iState;

	WriteInfoSeg(afcFile); /* 管理情報セグメントの書き込み */
	WriteFigStartSeg(afcFile); /* 図形開始セグメントの書き込み */

	iState=VDST_NORMAL;
	vdCurrent=document.vdFirst;
	while(vdCurrent!=NULL)
	{
		WriteVirtualObjSeg(afcFile,vdCurrent,&iState);
		vdCurrent=vdCurrent->next;
	}

	document.vdLastInDisk=document.vdLast;
	document.lLastOffset=afcFile->offset;
	document.iLinkNum=document.iVobjNum;
	WriteFigEndSeg(afcFile); /* 図形終了セグメントの書き込み */

	trc_rec(afcFile->fd,afcFile->offset); /* レコードの長さを正す */

	return TRUE;
}

BOOL SaveAllFile()
{
	LONG lTADRecode;
	FILECONTEXT* fcFile;

	gset_ptr(PS_BUSY,NULL,-1L,-1L); /* ポインタをゆのみにする */

	/* 機能付箋レコードがあるなら消す */
	if(fnd_rec(document.wFileDscr,F_TOPEND,128L,0,(LONG*)NULL)==7)
		while(fnd_rec(document.wFileDscr,F_FWD,128L,0,(LONG*)NULL)==7)
			del_rec(document.wFileDscr);

	if(fnd_rec(document.wFileDscr,F_TOPEND,0x00000002L,0,&lTADRecode)<0)
									/* 主TADレコードが見つからなかったら */
	apd_rec(document.wFileDscr,NULL,0L,1,0,0); /* 主TADレコード作成 */
	fcFile=StartFile(document.wFileDscr,lTADRecode);
	if(fcFile==NULL)
		return FALSE;

	/* 主TAD レコードの書き込み */
	if(!WriteTAD(fcFile))
		return FALSE;

	EndFile(fcFile);

	/* 余分な主TADレコードがあるなら消す */
	if(fnd_rec(document.wFileDscr,F_NFWD,0x00000002L,0,(LONG*)NULL)==1)
		while(fnd_rec(document.wFileDscr,F_FWD,0x00000002L,0,(LONG*)NULL)==1)
			del_rec(document.wFileDscr);

	/* リンクレコードの消去・書き込み */
	if(!WriteLinkRecode(document.wFileDscr,document.idVirtualObj))
		return FALSE;

	document.iModified=NOSAVE;

	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* ポインタを元に戻す */

	return TRUE;
}

/* 変化した仮身セグメントだけ書き込む */
BOOL SaveModifiedSegmentOnly(aisAllTAD)
BOOL aisAllTAD;
{
	LONG lTADRecode;
	FILECONTEXT* fcFile;
	VOBJDATA* vdCurrent;

	gset_ptr(PS_BUSY,NULL,-1L,-1L); /* ポインタをゆのみにする */
	/* TAD主レコードを探す */
	if(fnd_rec(document.wFileDscr,F_TOPEND,0x00000002L,0,&lTADRecode)<0)
		return FALSE;

	fcFile=StartFile(document.wFileDscr,lTADRecode);
	if(fcFile==NULL)
		return FALSE;

	vdCurrent=document.vdFirst;
	while(vdCurrent!=NULL)
	{
		if(vdCurrent->iModified&(CHANGEORIGINALDATA|CHANGESEGMENT) &&
			 !aisAllTAD)
		{ /* 変化のあった仮身なら */
			VFPTR vfData;
			UWORD wSize;

			oget_vob(vdCurrent->idVirtualObj,NULL,NULL,0,&wSize);
			if(get_lmb(&vfData,(LONG)wSize,NOCLR)<0)
			{
				ErrPanel(0,FALSE);
				EndFile(fcFile);
				return FALSE;
			}
			oget_vob(vdCurrent->idVirtualObj,(VLINKPTR)NULL,
						vfData,wSize,(UWORD*)NULL);
			vdCurrent->wSize=wSize;
			see_rec(fcFile->fd,fcFile->TADRecode,1,(LPTR)NULL);
			if(!SeekFile(fcFile,vdCurrent->lOffset))
			{
				ErrPanel(ID_ST_NWDATA,FALSE);
				EndFile(fcFile);
				return FALSE;
			}

			if(!WriteData(fcFile,vfData.vobj,vdCurrent->wSize)) /* 書き込み */
			{
				ErrPanel(ID_ST_NWDATA,FALSE);
				EndFile(fcFile);
				return FALSE;
			}
			vdCurrent->iModified&=~(CHANGEORIGINALDATA|CHANGESEGMENT);
			rel_lmb(vfData);
		}
		if(!vdCurrent->isFusen && vdCurrent->iModified&CHANGELINK)
		{ /* リンクレコードが変更されているなら */
			VLINK lnkLink;

			ocnv_vob(document.idVirtualObj,vdCurrent->idVirtualObj,&lnkLink);
			see_rec(fcFile->fd,vdCurrent->lRecode,1,(LPTR)NULL);
			if(wri_rec(fcFile->fd,0L,&lnkLink,sizeof(VLINK),
							(LPTR)NULL,(UWORD*)NULL,0)<0)
			{
				ErrPanel(ID_ST_NWLINK,FALSE);
				EndFile(fcFile);
				return FALSE;
			}
			vdCurrent->iModified&=~CHANGELINK;
		}
		if(vdCurrent==document.vdLastInDisk)
			break;
		vdCurrent=vdCurrent->next;
	}

	if(aisAllTAD)
	{
		SeekFile(fcFile,0L);
		if(!WriteTAD(fcFile))
		{
			EndFile(fcFile);
			return FALSE;
		}
	}else if((document.iModified&NEWDATA) && !(document.iModified&ILLEGALTAD))
	{ /* 追加書き込み */
		VLINK* p;
		int i;
		int iState;

		if(document.lnkInDisk!=NULL)
			rsz_lmb(&p,document.lnkInDisk,
					(LONG)(sizeof(VLINK)*document.iVobjNum),NOCLR);
		else
			get_lmb(&p,(LONG)(sizeof(VLINK)*document.iVobjNum),NOCLR);
		if(p==NULL)
		{
			ErrPanel(0,FALSE);
			EndFile(fcFile);
			return FALSE;
		}
		document.lnkInDisk=p;
		if(!SeekFile(fcFile,document.lLastOffset))
		{
			EndFile(fcFile);
			return FALSE;
		}

		iState=VDST_NORMAL;
		if(!WriteFigAplSegment(fcFile,iState))
		{
			EndFile(fcFile);
			return FALSE;
		}
		vdCurrent=vdCurrent->next;
		for(i=document.iLinkNum; vdCurrent!=NULL; vdCurrent=vdCurrent->next)
		{
			see_rec(fcFile->fd,fcFile->TADRecode,1,(LPTR)NULL);
			if(!WriteVirtualObjSeg(fcFile,vdCurrent,&iState))
			{								/* 仮身セグメントの書き込み */
				EndFile(fcFile);
				return FALSE;
			}
			if(!vdCurrent->isFusen)
			{
				ocnv_vob(document.idVirtualObj,vdCurrent->idVirtualObj,
												&(document.lnkInDisk[i]));
				/* 最後のレコードにリンクレコードを追加する */
				if(apd_rec(document.wFileDscr,&(document.lnkInDisk[i]),
							(LONG)sizeof(VLINK),0,0,0)<0)
				{
					ErrPanel(ID_ST_NWLINK,FALSE);
					EndFile(fcFile);
					return FALSE;
				}
				see_rec(fcFile->fd,-1L,-1,&(vdCurrent->lRecode));
				i++;
			}
		}
		document.lLastOffset=fcFile->offset;
		document.vdLastInDisk=document.vdLast;
		document.iLinkNum=document.iVobjNum;
		see_rec(fcFile->fd,fcFile->TADRecode,1,(LPTR)NULL);
		if(!WriteFigEndSeg(fcFile))
		{
			ErrPanel(ID_ST_NWDATA,FALSE);
			EndFile(fcFile);
			return FALSE;
		}
	}

	if(!EndFile(fcFile))
		return FALSE;
	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* ポインタを元に戻す */

	document.iModified=NOSAVE;

	return TRUE;
}


/* ファイルのセーブ */
BOOL SaveFile()
{
	if(document.isReadOnly) /* 読み込み専用なら */
	{
		ErrPanel(ID_ST_NWRONL,FALSE);
		return TRUE;
	}

	if( (document.iModified&ALLSAVE) || (document.iModified&ILLEGALFILE)&&
		(document.iModified&(CHANGEORIGINALDATA|CHANGESEGMENT|
							 NEWDATA|CHANGELINK|SAVETAD)) )
										/* ファイルが変更されているなら */
	{
		if(!SaveAllFile())
		{
			ErrPanel(ID_ST_NWFILE,FALSE);
			return FALSE;
		}
	}else if( (document.iModified&SAVETAD)&&!((document.iModified&ILLEGALTAD)&&
		(document.iModified&(CHANGEORIGINALDATA|CHANGESEGMENT|NEWDATA))) )
													/* TAD だけセーブする時 */
	{
		if(!SaveModifiedSegmentOnly(TRUE))
		{
			ErrPanel(ID_ST_NWFILE,FALSE);
			return FALSE;
		}
	}else if(document.iModified&
					(CHANGEORIGINALDATA|CHANGESEGMENT|CHANGELINK|NEWDATA))
	{ /* 仮身セグメントだけ変更されているなら */
		if(!SaveModifiedSegmentOnly(FALSE))
		{
			ErrPanel(ID_ST_NWFILE,FALSE);
			return FALSE;
		}
	}
	return TRUE;
}


BOOL SaveNewFile()
{
	WORD idNewVobj,idOldVobj;
	WORD wNewFD,wOldFD;
	TCODE tcName[21];

	tcName[0]=TK_NULL;
	/* 新版の作成 */
	wNewFD=ocre_obj(document.idVirtualObj,tcName,
						&idNewVobj,(LINKPTR)NULL,1);
	if(wNewFD<0)
	{
		ErrPanel(ID_ST_NMKNEWFILE,FALSE);
		return FALSE;
	}

	idOldVobj=document.idVirtualObj;
	wOldFD=document.wFileDscr;
	document.idVirtualObj=idNewVobj;
	document.wFileDscr=wNewFD;

	if(!SaveAllFile())
	{
		ErrPanel(ID_ST_NWFILE,FALSE);
		document.idVirtualObj=idOldVobj;
		document.wFileDscr=wOldFD;
		return FALSE;
	}

	cls_fil(wNewFD);

	document.idVirtualObj=idOldVobj;
	document.wFileDscr=wOldFD;

	document.iModified|=ILLEGALTAD;
						/* セーブするときには全体セーブをしなければならない。
						   というのも、SaveAllFile() で部分セーブの情報が
						   書き変わってしまったから。 */
	return TRUE;
}


/* --------------------データ管理-------------------- */


void UnlinkVobjData(avdUnlink)
VOBJDATA* avdUnlink;
{
	if(avdUnlink->next!=NULL && avdUnlink->previous!=NULL)
	{
		((avdUnlink->next)->previous) = (avdUnlink->previous);
		((avdUnlink->previous)->next) = (avdUnlink->next);
	}else if(avdUnlink->next==NULL && avdUnlink->previous!=NULL)
	{
		((avdUnlink->previous)->next) = NULL;
		document.vdLast=avdUnlink->previous;
	}else if(avdUnlink->next!=NULL && avdUnlink->previous==NULL)
	{
		((avdUnlink->next)->previous) = NULL;
		document.vdFirst=avdUnlink->next;
	}else if(avdUnlink->next==NULL && avdUnlink->previous==NULL)
	{
		document.vdFirst=NULL;
		document.vdLast=NULL;
	}
}

/* データ全体の描画領域を設定し直す */
void UpdateDocViewRect()
{
	VOBJDATA* vdCurrent;

	document.rctView.l[0]=0;
	document.rctView.l[1]=0;
	vdCurrent=document.vdFirst;
	while(vdCurrent!=NULL)
	{
		CheckViewSize(vdCurrent->rctView);
		vdCurrent=vdCurrent->next;
	}
}

BOOL IsDataModified() /* 変更パネルを表示しなければならないかどうか */
{
	if( document.iModified&(ALLSAVE|SAVETAD|NEWDATA|CHANGESEGMENT) &&
		!(document.iModified&SILENTSAVE) )
		return TRUE;
	else
		return FALSE;
}

BOOL IsSegmentOnlyModified()
{
	if(!IsDataModified() && (document.iModified&SILENTSAVE) )
		return TRUE;
	else
		return FALSE;
}

/* 次の2つはできることなら使ってはいけない関数 */
int SaveModification()
{
	return document.iModified;
}
void RecoverModification(aiModified)
int aiModified;
{
	document.iModified=aiModified;
}

void ForceNoSave()
{
	document.iModified=NOSAVE;
}

BOOL IsFusen(adiCurrent)
DOCITERATOR adiCurrent;
{
	return ((VOBJDATA*)adiCurrent)->isFusen;
}

DOCITERATOR GetIterator(aidVobj)
WORD aidVobj;
{
	VOBJDATA* vdCurrent;

	vdCurrent=document.vdFirst;
	while(vdCurrent!=NULL)
	{
		if(vdCurrent->idVirtualObj==aidVobj)
			return (DOCITERATOR)vdCurrent;
		vdCurrent=vdCurrent->next;
	}
	return NULL;
}

int GetVobjNum(adiCurrent)
DOCITERATOR adiCurrent;
{
	return document.iVobjNum+document.iFusenNum;
}

DOCITERATOR GetNext(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(adiCurrent==NULL)
		return document.vdFirst;
	return ((VOBJDATA*)adiCurrent)->next;
}

DOCITERATOR GetPrevious(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(adiCurrent==NULL)
		return document.vdLast;
	return ((VOBJDATA*)adiCurrent)->previous;
}

WORD GetVID(adiCurrent)
DOCITERATOR adiCurrent;
{
	return ((VOBJDATA*)adiCurrent)->idVirtualObj;
}

RECT GetRect(adiCurrent)
DOCITERATOR adiCurrent;
{
	return ((VOBJDATA*)adiCurrent)->rctView;
}

void ChangeRect(adiCurrent,arctView)
DOCITERATOR adiCurrent;
RECT arctView;
{
	if( ((VOBJDATA*)adiCurrent)->iState != VDST_NORMAL )
		return;
	((VOBJDATA*)adiCurrent)->rctView=arctView;
	UpdateDocViewRect();
	((VOBJDATA*)adiCurrent)->iModified|=CHANGESEGMENT|CHANGELINK;
	document.iModified|=CHANGESEGMENT|CHANGELINK;
	document.iModified&=~SILENTSAVE;
}

void MoveRect(adiCurrent,awDx,awDy)
DOCITERATOR adiCurrent;
WORD awDx,awDy;
{
	VOBJDATA* vdCurrent;

	vdCurrent=(VOBJDATA*)adiCurrent;

	if(vdCurrent->iState!=VDST_NORMAL)
		return;

	vdCurrent->rctView.c.left+=awDx;
	vdCurrent->rctView.c.right+=awDx;
	vdCurrent->rctView.c.top+=awDy;
	vdCurrent->rctView.c.bottom+=awDy;
	if(vdCurrent->rctView.c.left<COORDINATE_MIN)
	{
		vdCurrent->rctView.c.right-=vdCurrent->rctView.c.left-COORDINATE_MIN;
		vdCurrent->rctView.c.left=COORDINATE_MIN;
	}
	if(vdCurrent->rctView.c.top<COORDINATE_MIN)
	{
		vdCurrent->rctView.c.bottom-=vdCurrent->rctView.c.top-COORDINATE_MIN;
		vdCurrent->rctView.c.top=COORDINATE_MIN;
	}
	vdCurrent->iModified|=CHANGESEGMENT;
	UpdateDocViewRect();
	document.iModified|=CHANGESEGMENT;
	document.iModified&=~SILENTSAVE;
}

RECT GetDocRect()
{
	return document.rctView;
}

BOOL IsFixed(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(((VOBJDATA*)adiCurrent)->iState==VDST_FIXED)
		return TRUE;
	return FALSE;
}

BOOL IsBackground(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(((VOBJDATA*)adiCurrent)->iState==VDST_BACKGROUND)
		return TRUE;
	return FALSE;
}

void UnBackgroundAllVobj()
{
	VOBJDATA* vdCurrent;

	UnSelectAllVobj();
	vdCurrent=(VOBJDATA*)GetNext(NULL);
	while(vdCurrent!=NULL)
	{
		if(vdCurrent->iState==VDST_BACKGROUND)
		{
			vdCurrent->iState=VDST_NORMAL;
			SelectVobj((DOCITERATOR)vdCurrent);
		}
		vdCurrent=(VOBJDATA*)GetNext(vdCurrent);
	}
	document.iModified|=ALLSAVE;
	document.iModified&=~SILENTSAVE;
}

void NormalizeSelectedVobj()
{
	VOBJDATA* vdCurrent;

	vdCurrent=(VOBJDATA*)GetNextSelectedVobj(NULL);
	while(vdCurrent!=NULL)
	{
		vdCurrent->iState=VDST_NORMAL;
		vdCurrent=(VOBJDATA*)GetNextSelectedVobj(vdCurrent);
	}
	document.iModified|=SAVETAD;
	document.iModified&=~SILENTSAVE;
}

void FixSelectedVobj(adiCurrent)
{
	VOBJDATA* vdCurrent;

	vdCurrent=(VOBJDATA*)GetNextSelectedVobj(NULL);
	while(vdCurrent!=NULL)
	{
		vdCurrent->iState=VDST_FIXED;
		vdCurrent=(VOBJDATA*)GetNextSelectedVobj(vdCurrent);
	}
	document.iModified|=SAVETAD;
	document.iModified&=~SILENTSAVE;
}

void BackgroundVobj(adiCurrent)
DOCITERATOR adiCurrent;
{
	((VOBJDATA*)adiCurrent)->iState=VDST_BACKGROUND; /* 背景化 */
	UnSelectVobj(adiCurrent);
	document.iModified|=ALLSAVE;
	document.iModified&=~SILENTSAVE;
}

int GetSelectedVobjNum()
{
	return document.iSelectedNum;
}

DOCITERATOR GetNextSelectedVobj(adiCurrent)
DOCITERATOR adiCurrent;
{
	VOBJDATA* vdCurrent;

	vdCurrent=adiCurrent;
	if(adiCurrent==NULL)
		vdCurrent=document.vdFirst;
	else
		vdCurrent=((VOBJDATA*)adiCurrent)->next;

	while(vdCurrent!=NULL)
	{
		if(vdCurrent->isSelect==TRUE)
			return vdCurrent;
		vdCurrent=vdCurrent->next;
	}
	return NULL; /* 見つからなかったとき */
}

BOOL IsSelected(adiCurrent)
DOCITERATOR adiCurrent;
{
	return ((VOBJDATA*)adiCurrent)->isSelect;
}

void SelectVobj(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(IsBackground(adiCurrent))
		return;
	if(((VOBJDATA*)adiCurrent)->isSelect==FALSE)
	{
		document.iSelectedNum++;
		((VOBJDATA*)adiCurrent)->isSelect=TRUE;
	}
}

void SwitchVobjSelect(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(((VOBJDATA*)adiCurrent)->isSelect==FALSE)
	{
		document.iSelectedNum++;
		((VOBJDATA*)adiCurrent)->isSelect=TRUE;
	}else{
		document.iSelectedNum--;
		((VOBJDATA*)adiCurrent)->isSelect=FALSE;
	}
}

void UnSelectVobj(adiCurrent)
DOCITERATOR adiCurrent;
{
	if(((VOBJDATA*)adiCurrent)->isSelect==TRUE)
	{
		document.iSelectedNum--;
		((VOBJDATA*)adiCurrent)->isSelect=FALSE;
	}
}

void UnSelectAllVobj()
{
	VOBJDATA* vdCurrent;

	vdCurrent=document.vdFirst;
	while(vdCurrent!=NULL)
	{
		vdCurrent->isSelect=FALSE;
		vdCurrent=vdCurrent->next;
	}
	document.iSelectedNum=0;
}

void SelectAllVobj()
{
	DOCITERATOR* diCurrent;

	diCurrent=GetNext(NULL);
	while(diCurrent!=NULL)
	{
		SelectVobj(diCurrent);
		diCurrent=GetNext(diCurrent);
	}
}

void VobjSegmentModified(aidVobj)
int aidVobj;
{
	VOBJDATA* vdCurrent;
	UWORD wSize;

	vdCurrent=(VOBJDATA*)GetIterator(aidVobj);
	oget_vob(aidVobj,(VLINKPTR)NULL,(VOBJSEG*)NULL,0,&wSize);
	if(wSize!=vdCurrent->wSize) /* 仮身セグメントのサイズか変わっていたなら */
	{
		document.iModified|=SAVETAD|SILENTSAVE;
		if(!vdCurrent->isFusen)
			vdCurrent->iModified|=CHANGELINK;
	}else{
		if(vdCurrent->isFusen)
		{
			vdCurrent->iModified|=CHANGEORIGINALDATA;
			document.iModified|=CHANGEORIGINALDATA|SILENTSAVE;
		}else{
			vdCurrent->iModified|=CHANGEORIGINALDATA|CHANGELINK;
			document.iModified|=CHANGEORIGINALDATA|CHANGELINK|SILENTSAVE;
		}
	}
}

void ChangedVirtualObjRelation(adiChanged)
DOCITERATOR adiChanged;
{
	((VOBJDATA*)adiChanged)->iModified|=CHANGELINK;
	document.iModified|=CHANGELINK;
	document.iModified&=~SILENTSAVE;
}

void DeleteData(adiDelete)
DOCITERATOR adiDelete;
{
	VOBJDATA* vdDelete;

	vdDelete=(VOBJDATA*)adiDelete;
	if(vdDelete->iState!=VDST_NORMAL)
		return;
	odel_vob(vdDelete->idVirtualObj,0); /* 仮身の登録削除 */

	if(vdDelete->isSelect)
		document.iSelectedNum--;
	if(vdDelete->isFusen)
		document.iFusenNum--;
	else
		document.iVobjNum--;

	if(vdDelete->isFusen)
		document.iModified|=SAVETAD;
	else
		document.iModified|=ALLSAVE;

	UnlinkVobjData(vdDelete);

	rel_lmb(vdDelete); /* メモリの解放 */

	UpdateDocViewRect();

	document.iModified&=~SILENTSAVE;
}

void TakeFrontOrBackSelectedData(aisFront)
BOOL aisFront; /* TRUE=いちばん前へ, FALSE=いちばん後ろへ */
{
	int i;
	VOBJDATA* vdCurrent;

	vdCurrent=(VOBJDATA*)GetNextSelectedVobj(NULL);
	for(i=0; i<document.iSelectedNum; i++)
	{
		VOBJDATA* vdNext;

		vdNext=(VOBJDATA*)GetNextSelectedVobj((DOCITERATOR)vdCurrent);
		UnlinkVobjData(vdCurrent);
		AddVobjData(vdCurrent,aisFront);

		if(vdCurrent->isFusen)
			document.iModified|=SAVETAD;
		else
			document.iModified|=ALLSAVE;

		vdCurrent=vdNext;
	}
	document.iModified&=~SILENTSAVE;
}

RECT GetRectSurroundingSelectedData()
{
	RECT rctDraw;
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	rctDraw=GetRect(diCurrent);
	while(diCurrent!=NULL)
	{
		RECT rctDataView;

		rctDataView=GetRect(diCurrent);
		if(rctDataView.c.left<rctDraw.c.left)
			rctDraw.c.left=rctDataView.c.left;
		if(rctDataView.c.top<rctDraw.c.top)
			rctDraw.c.top=rctDataView.c.top;
		if(rctDataView.c.right>rctDraw.c.right)
			rctDraw.c.right=rctDataView.c.right;
		if(rctDataView.c.bottom>rctDraw.c.bottom)
			rctDraw.c.bottom=rctDataView.c.bottom;
		diCurrent=GetNextSelectedVobj(diCurrent);
	}

	return rctDraw;
}

POINT SendSelectedDataToTray(aisTempTray,aptOrigin)
BOOL aisTempTray;
POINT aptOrigin;
{
	int iSelectedNum;
	TRAYREC* ptrTrayIndex;
	FIGSEG fsFigStart;
	int i;
	DOCITERATOR diCurrent;
	POINT ptReturn;

	iSelectedNum=GetSelectedVobjNum();

	/* メモリ割り当て */
	if(get_lmb(&ptrTrayIndex,(LONG)(sizeof(TRAYREC)*(iSelectedNum+1)),NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		ptReturn.c.h=0;
		ptReturn.c.v=0;
		return ptReturn;
	}

	/* 最初に来る図形セグメントの設定 */
	fsFigStart.draw=GetRectSurroundingSelectedData();
	fsFigStart.view.c.left=0;
	fsFigStart.view.c.top=0;
	fsFigStart.view.c.right=fsFigStart.draw.c.right-fsFigStart.draw.c.left;
	fsFigStart.view.c.bottom=fsFigStart.draw.c.bottom-fsFigStart.draw.c.top;
	fsFigStart.h_unit=document.wHorizontalReso;
	fsFigStart.v_unit=document.wVerticalReso;
	fsFigStart.ratio=0;
	ptrTrayIndex[0].id=TS_FIG;
	ptrTrayIndex[0].len=(LONG)sizeof(FIGSEG);
	ptrTrayIndex[0].dt=&fsFigStart;

	diCurrent=GetNextSelectedVobj(NULL);
	for(i=1; i<=iSelectedNum; i++)
	{
		TR_VOBJREC* ptvVobj;
		FUSENSEG* pfsFusen;
		VLINK vlnkLink;
		UWORD wSize;
		WORD wVobjRecSize;
		WORD wErr;

		oget_vob(GetVID(diCurrent),(VLINKPTR*)NULL,(VFPTR*)NULL,0,&wSize);
		wVobjRecSize=sizeof(TR_VOBJREC)+wSize-sizeof(VOBJSEG);
		if(IsFusen(diCurrent))
			wErr=get_lmb(&pfsFusen,(LONG)wVobjRecSize,NOCLR);
		else
			wErr=get_lmb(&ptvVobj,(LONG)wVobjRecSize,NOCLR);
		if(wErr<0)
		{
			ErrPanel(0,FALSE);
			rel_lmb(ptrTrayIndex);
			ptReturn.c.h=0;
			ptReturn.c.v=0;
			return ptReturn;
		}
		if(IsFusen(diCurrent))
		{
			oget_vob(GetVID(diCurrent),NULL,
						pfsFusen,wSize,(UWORD*)NULL);
			ptrTrayIndex[i].id=TS_FFUSEN;
			ptrTrayIndex[i].len=(LONG)wVobjRecSize;
			ptrTrayIndex[i].dt=pfsFusen;
		}else{
			oget_vob(GetVID(diCurrent),&(ptvVobj[0].vlnk),
						&(ptvVobj[0].vseg),wSize,(UWORD*)NULL);
			ptrTrayIndex[i].id=TR_VOBJ;
			ptrTrayIndex[i].len=(LONG)wVobjRecSize;
			ptrTrayIndex[i].dt=ptvVobj;
		}

		diCurrent=GetNextSelectedVobj(diCurrent);
	}
	if(aisTempTray)
		tset_dat(ptrTrayIndex,iSelectedNum+1); /* 一時トレーにデータを送る */
	else{
		WORD wKey;
		TPTR ptMessage;

		wKey=dget_dat(TEXT_DATA,ID_ST_TRVOBJ,0); /* データの名称を得る */
		smb_adr(wKey,&ptMessage);
		tpsh_dat(ptrTrayIndex,iSelectedNum+1,ptMessage);
		drel_dat(wKey);
	}

	for(i=1; i<iSelectedNum; i++)
		rel_lmb(ptrTrayIndex[i].dt);
	rel_lmb(ptrTrayIndex);

	ptReturn.c.h=fsFigStart.draw.c.left-aptOrigin.c.h;
	ptReturn.c.v=fsFigStart.draw.c.top-aptOrigin.c.v;
	return ptReturn;
}

void PasteFromTray(aidWnd,aisTempTray,aptInsertPoint)
WORD aidWnd;
BOOL aisTempTray;
POINT aptInsertPoint;
{
	LONG lTraySize;
	WORD wRecodeNum;
	TRAYREC* ptrTrayIndex;
	int i;
	BOOL isStartSegCame;
	BOOL isText;

	/* トレーサイズを得る */
	if(aisTempTray)
		wRecodeNum=tget_dat((TRAYREC*)NULL,0L,&lTraySize,-1);
	else
		wRecodeNum=tpop_dat((TRAYREC*)NULL,0L,&lTraySize,-1,(TPTR)NULL);
	
	if(wRecodeNum==0) /* トレー内にデータがなければ */
			return; /* 何もせずに終わる */

	UnSelectAllVobj();

	/* メモリ割り当て */
	if(get_lmb(&ptrTrayIndex,lTraySize,NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		return;
	}

	/* データ取りだし */
	if(aisTempTray)
		wRecodeNum=tget_dat(ptrTrayIndex,lTraySize,(LONG*)NULL,-1);
	else
		wRecodeNum=tpop_dat(ptrTrayIndex,lTraySize,(LONG*)NULL,-1,(TPTR)NULL);

	isStartSegCame=FALSE;
	for(i=0; i<wRecodeNum; i++)
	{
		POINT ptTextInsertPoint;

/*b_printf("Paste ID=%d\n",ptrTrayIndex[i].id);*/
		if(ptrTrayIndex[i].id==TS_FIG && !isStartSegCame)
		{
			aptInsertPoint.c.h-=
					((FIGSEG*)(ptrTrayIndex[i].dt))->draw.c.left;
			aptInsertPoint.c.v-=
					((FIGSEG*)(ptrTrayIndex[i].dt))->draw.c.top;
			isText=FALSE;
			isStartSegCame=TRUE;
		}else if(ptrTrayIndex[i].id==TS_TEXT && !isStartSegCame)
		{
			ptTextInsertPoint.c.h=aptInsertPoint.c.h;
			ptTextInsertPoint.c.v=aptInsertPoint.c.v;
			isText=TRUE;
			isStartSegCame=TRUE;
		}else if( (ptrTrayIndex[i].id==TR_VOBJ||ptrTrayIndex[i].id==TS_FFUSEN)
					&& isStartSegCame )
		{
			VOBJDATA* pvdAdd;
			VFPTR vfItem;
			VLINK* pvlnkItem;
			WORD idVobj;

			if(ptrTrayIndex[i].id==TR_VOBJ)
			{
				vfItem.vobj=&(((TR_VOBJREC*)(ptrTrayIndex[i].dt))->vseg);
				pvlnkItem=&(((TR_VOBJREC*)(ptrTrayIndex[i].dt))->vlnk);
			}else{
				vfItem.fsn=((FUSENSEG*)(ptrTrayIndex[i].dt));
				pvlnkItem=NULL;
			}

			/* 挿入位置の調整 */
			if(isText)
			{
				vfItem.vobj->view.c.right=ptTextInsertPoint.c.h+
					vfItem.vobj->view.c.right-vfItem.vobj->view.c.left;
				vfItem.vobj->view.c.bottom=ptTextInsertPoint.c.v+
					vfItem.vobj->view.c.bottom-vfItem.vobj->view.c.top;
				vfItem.vobj->view.c.left=ptTextInsertPoint.c.h;
				vfItem.vobj->view.c.top=ptTextInsertPoint.c.v;
				ptTextInsertPoint.c.v=vfItem.vobj->view.c.bottom+5;
			}else{
				vfItem.vobj->view.c.left+=aptInsertPoint.c.h;
				vfItem.vobj->view.c.right+=aptInsertPoint.c.h;
				vfItem.vobj->view.c.top+=aptInsertPoint.c.v;
				vfItem.vobj->view.c.bottom+=aptInsertPoint.c.v;
			}
			/* 座標が負にならないようにする */
			if(vfItem.vobj->view.c.left<COORDINATE_MIN)
			{
				vfItem.vobj->view.c.right-=vfItem.vobj->view.c.left-
														COORDINATE_MIN;
				vfItem.vobj->view.c.left=COORDINATE_MIN;
			}
			if(vfItem.vobj->view.c.top<COORDINATE_MIN)
			{
				vfItem.vobj->view.c.bottom-=vfItem.vobj->view.c.top-
														COORDINATE_MIN;
				vfItem.vobj->view.c.top=COORDINATE_MIN;
			}

			/* 仮身の登録 */
			idVobj=oreg_vob(pvlnkItem,vfItem,aidWnd,V_DISPALL);
			if(idVobj<0) /* 登録できなかったら */
			{
				ErrPanel(ID_ST_NREGVOBJ,FALSE);
				rel_lmb(ptrTrayIndex);
				return;
			}

			/* 項目へメモリー割り当て */
			if(get_lmb(&pvdAdd,(LONG)sizeof(VOBJDATA),NOCLR)<0)
			{
				ErrPanel(0,FALSE);
				rel_lmb(ptrTrayIndex);
				return;
			}

			/* 項目の要素をセット */
			pvdAdd->rctView=vfItem.vobj->view;
			pvdAdd->idVirtualObj=idVobj;
			pvdAdd->isSelect=TRUE;
			pvdAdd->iModified=NEWDATA;
			pvdAdd->lOffset=0;
	 		pvdAdd->wSize=0;
	 		pvdAdd->lRecode=0;
			if(ptrTrayIndex[i].id==TR_VOBJ)
			{
				pvdAdd->isFusen=FALSE;
			}else{
				pvdAdd->isFusen=TRUE;
			}

			AddVobjData(pvdAdd,TRUE);

			document.iSelectedNum++;
		}
	}
	document.iModified|=NEWDATA;
	document.iModified&=~SILENTSAVE;

	rel_lmb(ptrTrayIndex);
}



/* --------------------固有データの処理-------------------- */


#define UNIQUEDATASIZE 8 /* 固有データの大きさ */
/* 実行機能付箋からの固有データの読み込み */
BOOL ReadUniqueData(awUniqueData,aprctWnd,aprctWork)
WORD* awUniqueData;
RECT* aprctWnd;
RECT* aprctWork;
{
	WORD* wData;
	WORD wBuf[UNIQUEDATASIZE+1];

	if(awUniqueData==NULL)
	{
		wData=wBuf;
		oget_fsn(document.idVirtualObj,document.wFileDscr,wData,
				(UWORD)(UNIQUEDATASIZE+1)*sizeof(WORD));
	}else
		wData=awUniqueData;

	if(wData[0]!=UNIQUEDATASIZE*sizeof(WORD))
		return FALSE;

	aprctWnd->c.left=wData[1];
	aprctWnd->c.top=wData[2];
	aprctWnd->c.right=wData[3];
	aprctWnd->c.bottom=wData[4];
	aprctWork->c.left=wData[5];
	aprctWork->c.top=wData[6];
	aprctWork->c.right=wData[7];
	aprctWork->c.bottom=wData[8];

	return TRUE;
}

void WriteUniqueData(arctWnd,arctWork)
RECT arctWnd;
RECT arctWork;
{
	WORD wData[UNIQUEDATASIZE+1];

	wData[0]=UNIQUEDATASIZE*sizeof(WORD);
	wData[1]=arctWnd.c.left;
	wData[2]=arctWnd.c.top;
	wData[3]=arctWnd.c.right;
	wData[4]=arctWnd.c.bottom;
	wData[5]=arctWork.c.left;
	wData[6]=arctWork.c.top;
	wData[7]=arctWork.c.right;
	wData[8]=arctWork.c.bottom;

	if(!document.isReadOnly)
		oput_fsn(document.idVirtualObj,document.wFileDscr,wData);

	oend_prc(document.idVirtualObj,wData,0); /* 実身の処理終了 */
}
