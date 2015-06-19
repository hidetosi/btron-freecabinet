#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"

WORD idBar[2];


BOOL SetBarValue(aidWnd)
WORD aidWnd;
{
	WORD value[4];
	RECT rctWnd,rctDoc;
	static RECT rctOldWnd={0,0,0,0},rctOldDoc={0,0,0,0};
	
	rctDoc=GetDocRect();
	rctDoc.c.top=0;
	rctDoc.c.left=0;
	wget_wrk(aidWnd,&rctWnd);

	if(rctWnd.c.top!=rctOldWnd.c.top || rctWnd.c.bottom!=rctOldWnd.c.bottom ||
	   rctDoc.c.top!=rctOldDoc.c.top || rctDoc.c.bottom!=rctOldDoc.c.bottom)
	{
		value[0]=rctWnd.c.top;
		value[1]=rctWnd.c.bottom;
		value[2]=rctDoc.c.top;
		value[3]=rctDoc.c.bottom;
		if(value[0]<value[2])
			value[2]=value[0];
		if(value[1]>value[3])
			value[3]=value[1];

		cset_val(idBar[RIGHTBAR],4,value);
	}
	if(rctWnd.c.left!=rctOldWnd.c.left ||
	   rctWnd.c.right!=rctOldWnd.c.right ||
	   rctDoc.c.left!=rctOldDoc.c.left ||
	   rctDoc.c.right!=rctOldDoc.c.right)
	{
		value[0]=rctWnd.c.right;
		value[1]=rctWnd.c.left;
		value[2]=rctDoc.c.right;
		value[3]=rctDoc.c.left;
		if(value[0]>value[2])
			value[2]=value[0];
		if(value[1]<value[3])
			value[3]=value[1];

		cset_val(idBar[BOTTOMBAR],4,value);
	}
	rctOldWnd=rctWnd;
	rctOldDoc=rctDoc;
	return TRUE;
}


BOOL InitScrollBar(aidWnd)
WORD aidWnd;
{
	/* スクロールバーのパーツ ID を取り出す */
	wget_bar(aidWnd,&idBar[RIGHTBAR],&idBar[BOTTOMBAR],(WORD*)NULL);

	SetBarValue(aidWnd);

	return TRUE;
}


void ScrollByBarDifference(aiBar,apwOld,apwNew,aidWnd)
int aiBar;
WORD *apwOld,*apwNew;
WORD aidWnd;
{
	WORD idDrawEnv;
	WORD wMove;
	WORD err;
	RECT rctWork;

	if(apwNew[0]==apwOld[0])
		return;

	idDrawEnv=wget_gid(aidWnd); /* 描画環境の取得 */
	wget_wrk(aidWnd,&rctWork);
	gset_vis(idDrawEnv,rctWork); /* 領域のセット */

	wMove=apwOld[0]-apwNew[0];
	if(aiBar==RIGHTBAR)
		err=wscr_wnd(aidWnd,(RECT*)NULL,0,wMove,W_SCRL|W_RDSET);
	else
		err=wscr_wnd(aidWnd,(RECT*)NULL,wMove,0,W_SCRL|W_RDSET);

	SetBarValue(aidWnd);

	if(err&W_RDSET) /* 再表示が必要なら */
		DrawView(aidWnd);
}



void BarMove(aiBar,aiDirection,aiSmoothOrArea,aidWnd,aisEnlarge)
int aiBar;
int aiDirection; /* 移動方向 1:正 -1:負 */
WORD aiSmoothOrArea; /* P_SMOOTH  P_AREA */
WORD aidWnd;
BOOL aisEnlarge;
{
	static wHeight=0; /* 文字の高さ */
	WORD pwOld[4];
	WORD pwBarValue[4];
	int iKnobSize;

	if(wHeight==0)
	{
		TCODE DevName[11];
		DEV_SPEC spec;
		FONTINF info;
		WORD wGid;

		wGid=wget_gid(aidWnd);
		gget_dev(wGid,DevName);
		gget_spc(DevName,&spec);
		gget_fnt(wGid,NULL,&info);
		wHeight=info.height;
	}

	cget_val(idBar[aiBar],4,pwOld);
	pwBarValue[0]=pwOld[0];
	pwBarValue[1]=pwOld[1];
	pwBarValue[2]=pwOld[2];
	pwBarValue[3]=pwOld[3];

	iKnobSize=pwOld[0]-pwOld[1];
	if(iKnobSize<0)
		iKnobSize=-iKnobSize;

	if(aiSmoothOrArea==P_SMOOTH)
	{
		pwBarValue[0]+=wHeight*aiDirection;
		pwBarValue[1]+=wHeight*aiDirection;
	}else{
		pwBarValue[0]+=iKnobSize*aiDirection;
		pwBarValue[1]+=iKnobSize*aiDirection;
	}

	if(pwBarValue[0]>pwBarValue[2] && aiBar==BOTTOMBAR && !aisEnlarge)
	{ /* 右端を越えた */
		pwBarValue[0]=pwBarValue[2];
		pwBarValue[1]=pwBarValue[2]-iKnobSize;
	}else
	if(pwBarValue[0]<pwBarValue[2] && aiBar==RIGHTBAR && !aisEnlarge)
	{ /* 上端を越えた */
		pwBarValue[0]=pwBarValue[2];
		pwBarValue[1]=pwBarValue[2]+iKnobSize;
	}else
	if(pwBarValue[1]<pwBarValue[3] && aiBar==BOTTOMBAR && !aisEnlarge)
	{ /* 左端を越えた */
		pwBarValue[1]=pwBarValue[3];
		pwBarValue[0]=pwBarValue[3]+iKnobSize;
	}else
	if(pwBarValue[1]>pwBarValue[3] && aiBar==RIGHTBAR && !aisEnlarge)
	{ /* 右端を越えた */
		pwBarValue[1]=pwBarValue[3];
		pwBarValue[0]=pwBarValue[3]-iKnobSize;
	}

	if(pwBarValue[0]<0 && aiBar==RIGHTBAR)
	{
		pwBarValue[0]=0;
		pwBarValue[1]=iKnobSize;
	}
	if(pwBarValue[1]<0 && aiBar==BOTTOMBAR)
	{
		pwBarValue[0]=iKnobSize;
		pwBarValue[1]=0;
	}

/*	cset_val(idBar[aiBar],4,pwBarValue);*/

	ScrollByBarDifference(aiBar,pwOld,pwBarValue,aidWnd);
}


void ActScrollBar(aiBar,aevWnd)
int aiBar;
WEVENT *aevWnd;
{
	UWORD ret;
	WORD pwOldBarValue[4];

	cget_val(idBar[aiBar],4,pwOldBarValue);
	while( ((ret=cact_par(idBar[aiBar],aevWnd))&0x6000) == 0x6000 )
	{ /* 中断の時 */
		if(ret&0x0001) /* 増加方向なら */
			BarMove(aiBar,1,P_SMOOTH,aevWnd->s.wid,FALSE);
		else /* 減少方向なら */
			BarMove(aiBar,-1,P_SMOOTH,aevWnd->s.wid,FALSE);
		aevWnd->s.type=EV_NULL;
	}
	if( (ret&0x100c) == 0x1004 ) /* エリア移動の時 */
	{
		if(ret&0x0001) /* 増加方向なら */
			BarMove(aiBar,1,P_AREA,aevWnd->s.wid,FALSE);
		else /* 減少方向なら */
			BarMove(aiBar,-1,P_AREA,aevWnd->s.wid,FALSE);
	}else if( (ret&0x100c) == 0x1000 ) /* スムーズ移動の時 */
	{
		if(ret&0x0001) /* 増加方向なら */
			BarMove(aiBar,1,P_SMOOTH,aevWnd->s.wid,FALSE);
		else /* 減少方向なら */
			BarMove(aiBar,-1,P_SMOOTH,aevWnd->s.wid,FALSE);
	}else if( (ret&0x100c) == 0x1008 ) /* ジャンプ移動の時 */
	{
		WORD pwNewBarValue[4];

		cget_val(idBar[aiBar],4,pwNewBarValue);
		ScrollByBarDifference(aiBar,pwOldBarValue,pwNewBarValue,aevWnd->s.wid);
	}
}
