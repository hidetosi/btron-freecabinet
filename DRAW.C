#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"


#define REDISPRECTNUM 10 /* 再表示用の矩形リストの数 */


/* -------------------- 選択枠関係 -------------------- */


SEL_LIST* slSelectFrameList=NULL; /* 選択枠リスト */

/* 選択枠の表示 */
void DrawSelectFrame(aidWnd,awMode,awDx,awDy)
WORD aidWnd; /* 値がマイナスなら描画環境 */
WORD awMode;
WORD awDx;
WORD awDy;
{
	WORD idDrawEnv;
	RECT rctWork;

	if(slSelectFrameList==NULL)
		return;
	if(aidWnd>0) /* ウインドウに描くとき */
	{
		idDrawEnv=wget_gid(aidWnd);
		wget_wrk(aidWnd,&rctWork);
		gset_vis(idDrawEnv,rctWork); /* 領域のセット */
	}else /* 描画環境に描くとき */
		idDrawEnv=-aidWnd;
	adsp_slt(idDrawEnv,slSelectFrameList,awMode,awDx,awDy); /* 選択枠を表示 */
}

BOOL MakeSelectFrame()
{
	int iSelectedNum,i;
	DOCITERATOR diVobj;

	iSelectedNum=GetSelectedVobjNum();
	if(iSelectedNum==0)
		return TRUE;
	if(get_lmb(&slSelectFrameList,(LONG)iSelectedNum*sizeof(SEL_LIST),NOCLR)<0)
	{
		ErrPanel(0,FALSE);
		return FALSE;
	}
	diVobj=GetNextSelectedVobj(NULL);
	i=0;
	while(diVobj!=NULL)
	{
		if(IsBackground(diVobj))
		{
			diVobj=GetNextSelectedVobj(diVobj);
			continue;
		}
		slSelectFrameList[i].next=&slSelectFrameList[i+1];
		if(IsFixed(diVobj))
			slSelectFrameList[i].rgn.sts=0x0100;
		else
			slSelectFrameList[i].rgn.sts=0x0000;
		slSelectFrameList[i].rgn.rgn.r=GetRect(diVobj);
		diVobj=GetNextSelectedVobj(diVobj);
		i++;
	}
	slSelectFrameList[i-1].next=NULL;

	return TRUE;
}

void FreeSelectFrame()
{
	if(slSelectFrameList!=NULL)
	{
		rel_lmb(slSelectFrameList);
		slSelectFrameList=NULL;
	}
}


/* -------------------- 描画関係 -------------------- */


void DrawOneVobj(aidVobj,aidWnd)
WORD aidVobj;
WORD aidWnd;
{
	WORD idDrawEnv;
	RECT rctWork;

	idDrawEnv=wget_gid(aidWnd); /* 描画環境の取得 */
	wget_wrk(aidWnd,&rctWork); /* 作業領域の取得 */
	gset_vis(idDrawEnv,rctWork); /* 領域のセット */
	odsp_vob(aidVobj,NULL,V_DISPALL); /* 化身の表示 */
}

void DrawOneRect(aidWnd,arctDraw)
WORD aidWnd;
RECT arctDraw;
{
	WORD idDrawEnv;
	BYTE* pVIDList,*p;
	DOCITERATOR diCurrent;

/* b_printf("view(%d,%d)-(%d,%d)\n",arctDraw.c.left,arctDraw.c.top,arctDraw.c.right,arctDraw.c.bottom); */
	idDrawEnv=wget_gid(aidWnd); /* 描画環境の取得 */
	gset_vis(idDrawEnv,arctDraw); /* 領域のセット */

	wera_wnd(aidWnd,&arctDraw); /* 背景色で塗りつぶす */
	/* ID リストの作成 */
	get_lmb(&pVIDList,
			(LONG)(GetVobjNum()*(sizeof(WORD)+sizeof(RECT))+sizeof(WORD)),
			NOCLR);
	diCurrent=NULL; p=pVIDList;
	while( (diCurrent=GetNext(diCurrent)) !=NULL )
	{
		RECT rctVobj;

		rctVobj=GetRect(diCurrent);
		if(!(rctVobj.c.right<arctDraw.c.left||rctVobj.c.left>arctDraw.c.right||
		   rctVobj.c.bottom<arctDraw.c.top||rctVobj.c.top>arctDraw.c.bottom))
		{ /* 表示領域に入っていたら */
			*((WORD*)p)=GetVID(diCurrent);
			p+=sizeof(WORD);
			*((RECT*)p)=arctDraw;
			p+=sizeof(RECT);
/* b_printf("ID %d (%d,%d)-(%d,%d)\n",GetVID(diCurrent),rctVobj.c.left,rctVobj.c.top,rctVobj.c.right,rctVobj.c.bottom); */
		}
	}
	*((WORD*)p)=0;

	/* 化身の表示 */
	odsp_vob(0x8000,pVIDList,V_DISPALL);

	rel_lmb(pVIDList);
}


/* 再表示処理をする */
void DrawView(aidWnd)
WORD aidWnd;
{
	static RECTLIST rlListItem[REDISPRECTNUM];
	static RLPTR prlList=NULL; /* 再表示用の矩形リスト */

	DrawSelectFrame(aidWnd,0,0,0);

	if(prlList==NULL)
	{ /* 再表示用の矩形リストの初期化 */
		int i;

		prlList=rlListItem;
		for(i=0; i<REDISPRECTNUM-1; i++)
			rlListItem[i].r_next=&rlListItem[i+1];
		rlListItem[REDISPRECTNUM].r_next=NULL;
	}

	do {
		int iRectNum,i;
		RECT rctDraw;

		iRectNum=wsta_dsp(aidWnd,&rctDraw,prlList); /* 再表示開始 */
		if(iRectNum==0)
			break;
		if(iRectNum>REDISPRECTNUM) /* 再表示の必要な矩形の数が多いとき */
			DrawOneRect(aidWnd,rctDraw);
		else /* 再表示の必要な矩形の数が少ないとき */
			for(i=0; i<iRectNum; i++)
				DrawOneRect(aidWnd,rlListItem[i].rcomp); /* 1つ1つ描く */
	}while(wend_dsp(aidWnd)>0);
	return;
}

void DrawSelectedData(aidWnd)
WORD aidWnd;
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		DrawOneRect(aidWnd,GetRect(diCurrent));
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
}
