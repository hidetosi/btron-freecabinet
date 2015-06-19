#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"

/* -------------------- イベント関係 -------------------- */


/* 選択されたデータの削除 */
void DeleteSelectedData(aidWnd,aisTemp)
WORD aidWnd;
BOOL aisTemp; /* TRUE=仮に登録されていた物を削除する */
{
	DOCITERATOR diDelete;
	int iModified;

	if(aisTemp)
		iModified=SaveModification();
	diDelete=GetNextSelectedVobj(NULL);
	while(diDelete!=NULL)
	{
		DOCITERATOR diNext;
		RECT rctDeletedView;

		diNext=GetNextSelectedVobj(diDelete);
		rctDeletedView=GetRect(diDelete);
		DeleteData(diDelete); /* データの削除 */
		DrawOneRect(aidWnd,rctDeletedView);
		diDelete=diNext;
	}
	if(aisTemp)
		RecoverModification(iModified);
}


/* マウスのボタンを押したところにどの仮身があるかを調べる */
DOCITERATOR FindVobj(aptClick,pwLocation)
POINT aptClick;
WORD* pwLocation;
{
	DOCITERATOR diCurrent;

	diCurrent=GetPrevious(NULL); /* 最後のデータから検索する */
	while(diCurrent!=NULL)
	{
		WORD wVID;

		(*pwLocation)=ofnd_vob(-GetVID(diCurrent),aptClick,&wVID);
		if(IsBackground(diCurrent))
		{
			if(wVID!=0 && *pwLocation==V_PICT)
				return diCurrent;
		}else{
			if(wVID!=0)
				return diCurrent; /* 見つかった */
		}
		diCurrent=GetPrevious(diCurrent);
	}
	return NULL; /* 見つからなかった */
}

/* 選択されたデータのコピー */
BOOL CopySelectedData(aidWnd,awDx,awDy)
WORD aidWnd;
WORD awDx,awDy;
{
	DOCITERATOR diCopy;
	int i;

	diCopy=GetNextSelectedVobj(NULL);
	for(i=0; i<GetSelectedVobjNum(); i++)
	{
		WORD idNewVobj;
		DOCITERATOR diNew;
		RECT rctNewView;

		idNewVobj=odup_vob(GetVID(diCopy));
		if(idNewVobj<0)
		{
			ErrPanel(ID_ST_NREGVOBJ,FALSE);
			return FALSE;
		}
		AddVobjByID(idNewVobj);
		diNew=GetIterator(idNewVobj);
		MoveRect(diNew,awDx,awDy); /* データの移動 */
		rctNewView=GetRect(diNew);
		omov_vob(idNewVobj,0,&rctNewView,V_DISPALL); /*登録した仮身の移動*/
		UnSelectVobj(diCopy);
		SelectVobj(diNew);
		diCopy=GetNextSelectedVobj(diCopy);
	}
	SetBarValue(aidWnd);
}

/* 選択されたデータの移動 */
void MoveSelectedData(aidWnd,awDx,awDy)
WORD aidWnd;
WORD awDx,awDy;
{
	DOCITERATOR diMove;

	diMove=GetNextSelectedVobj(NULL);
	while(diMove!=NULL)
	{
		RECT rctOldView;
		RECT rctNewView;

		rctOldView=GetRect(diMove);
		MoveRect(diMove,awDx,awDy); /* データの移動 */
		rctNewView=GetRect(diMove);
		omov_vob(GetVID(diMove),0,&rctNewView,V_NODISP); /*登録した仮身の移動*/
		DrawOneRect(aidWnd,rctOldView);
		DrawOneRect(aidWnd,GetRect(diMove));
		diMove=GetNextSelectedVobj(diMove);
	}
	SetBarValue(aidWnd);
}

MakeNewObject(aidWnd,awDx,awDy,aidAtherWnd)
WORD aidWnd;
WORD awDx,awDy;
WORD aidAtherWnd; /* 他のウインドウに送る時に使う。自分のウインドウの時は 0 */
{
	DOCITERATOR diMakeNew;
	int i;
	int iModified;

	if(aidAtherWnd!=0)
		iModified=SaveModification();

	diMakeNew=GetNextSelectedVobj(NULL);
	for(i=0; i<GetSelectedVobjNum(); i++)
	{
		WORD idNewVobj;
		DOCITERATOR diNew;
		RECT rctNewView,rctOldView;

		rctOldView=GetRect(diMakeNew);
		if(aidAtherWnd==0) /* 自分のウインドウに新版を作るとき */
			idNewVobj=onew_obj(GetVID(diMakeNew),(VLINKPTR)NULL);
		else{ /* 他のウインドウに新版を作るとき */
			VFPTR pvfVobj;
			VLINK vlnkLink;
			UWORD wSize;
			WORD idTmpAtherOld,idTmpAtherNew;

			oget_vob(GetVID(diMakeNew),(VLINKPTR*)NULL,(VFPTR*)NULL,0,&wSize);
			if(get_lmb(&pvfVobj,(LONG)wSize,NOCLR)<0)
			{
				ErrPanel(0,FALSE);
				return FALSE;
			}
			oget_vob(GetVID(diMakeNew),&vlnkLink,pvfVobj,wSize,(UWORD*)NULL);
			idTmpAtherOld=oreg_vob(&vlnkLink,pvfVobj,aidAtherWnd,V_NODISP);
			idTmpAtherNew=onew_obj(idTmpAtherOld,(VLINK*)NULL);
			oget_vob(idTmpAtherNew,&vlnkLink,pvfVobj,wSize,(UWORD*)NULL);
			odel_vob(idTmpAtherOld,0);
			odel_vob(idTmpAtherNew,0);
			idNewVobj=oreg_vob(&vlnkLink,pvfVobj,aidWnd,V_NODISP);
			rel_lmb(pvfVobj);
		}
		if(idNewVobj<0)
			return FALSE;
		AddVobjByID(idNewVobj);
		diNew=GetIterator(idNewVobj);
		ChangeRect(diNew,rctOldView);
		MoveRect(diNew,awDx,awDy); /* データの移動 */
		rctNewView=GetRect(diNew);
		if(aidAtherWnd==0) /* 登録した仮身の移動 */
			omov_vob(idNewVobj,0,&rctNewView,V_DISPALL);
		else
			omov_vob(idNewVobj,0,&rctNewView,V_NODISP);
		UnSelectVobj(diMakeNew);
		SelectVobj(diNew);
		diMakeNew=GetNextSelectedVobj(diMakeNew);
	}
	if(aidAtherWnd==0)
		SetBarValue(aidWnd);
	if(aidAtherWnd!=0)
		RecoverModification(iModified);
}


/* 化身上の位置とポインタの形状の対応。[0][n]がドラッグしてないとき。 */
/* [1][n]がドラッグ中の時 */
WORD wPointStile[2][9]=
{{PS_MOVE,PS_MOVE,PS_SELECT,PS_MOVE,PS_RSIZ,PS_RSIZ,PS_RSIZ,PS_RSIZ,PS_MOVE},
 {PS_GRIP,PS_GRIP,PS_GRIP,  PS_GRIP,PS_PICK,PS_PICK,PS_PICK,PS_PICK,PS_GRIP}};

/* 仮身のドラッグ移動 */
BOOL DragMove(aidWnd,aptStart,aisNewObject)
WORD aidWnd;
POINT aptStart;
BOOL aisNewObject; /* 新版作成かどうか */
{
	WORD idDrawEnv; /* ドラッグ描画環境 */
	POINT ptMouse,ptOldMouse;
	WEVENT evDragEvent; /* ドラッグ中のイベント */
	BOOL isCopy; /* copy か move か */
	BOOL isBreak; /* ドラッグが中断されたかどうか */
	ULONG lScrollTimer;

	/* ポインタの形状の変更 */
	gset_ptr(PS_GRIP,(PTRIMAGE*)NULL,-1L,-1L);

	isBreak=FALSE;
	ptOldMouse=aptStart;
	lScrollTimer=0;
	idDrawEnv=wsta_drg(aidWnd,0); /* ドラッグの開始(lock なし) */
	MakeSelectFrame(); /* 選択枠リストを作成 */
	DrawSelectFrame(-idDrawEnv,1,0,0); /* 枠の表示 */

	while(wget_drg(&ptMouse,&evDragEvent)!=EV_BUTUP)
	{
		POINT ptAbsolute; /* マウスのグローバル座標 */
		WORD idToWnd; /* マウスのあるウインドウ ID */

		if(evDragEvent.s.type==EV_KEYDWN &&
			evDragEvent.e.data.key.code==TK_CAN) /* ×キーが押されたとき */
		{
			isBreak=TRUE;
			break; /* ドラッグの中断 */
		}
		if(ptMouse.l==ptOldMouse.l)
		{
			LONG lNow;

			get_etm(&lNow);
			if(!(lScrollTimer!=0 && lNow-lScrollTimer>300))
			{
				ptOldMouse=ptMouse;
				continue;
			}
		}
		/* マウスのあるウインドウIDを得る */
		ptAbsolute=ptMouse;
		gcnv_abs(idDrawEnv,&ptAbsolute); /* 絶対座標に変換 */
		wfnd_wnd(&ptAbsolute,(POINT*)NULL,&idToWnd);

		DrawSelectFrame(-idDrawEnv,0,ptOldMouse.c.h-aptStart.c.h,
						ptOldMouse.c.v-aptStart.c.v); /* 枠の消去 */

		if(idToWnd!=aidWnd) /* マウスがウインドウの外に出たなら */
		{
			RECT rctAllDisplay;

			lScrollTimer=0;
			gloc_env(idDrawEnv,1); /* ドラッグ描画環境のロック */
			/* 全画面を表示長方形にする */
			gget_fra(idDrawEnv,&rctAllDisplay);
			gset_vis(idDrawEnv,rctAllDisplay);
		}else{ /* マウスが元のウインドウの中の時 */
			RECT rctWndFrame;
			ULONG lNow;

			gloc_env(idDrawEnv,0); /* ドラッグ描画環境のアンロック */
			/* スクロール */
			if(rctWndFrame.c.left>ptMouse.c.h ||
				rctWndFrame.c.right<ptMouse.c.h ||
				rctWndFrame.c.top>ptMouse.c.v ||
				rctWndFrame.c.bottom<ptMouse.c.v)
			{
				if(lScrollTimer==0)
					get_etm(&lScrollTimer);
				get_etm(&lNow);
				if(lNow-lScrollTimer>300)
				{
					wget_wrk(aidWnd,&rctWndFrame);
					if(rctWndFrame.c.left>ptMouse.c.h)
						BarMove(BOTTOMBAR,-1,P_SMOOTH,aidWnd,TRUE);
					if(rctWndFrame.c.right<ptMouse.c.h)
						BarMove(BOTTOMBAR,1,P_SMOOTH,aidWnd,TRUE);
					if(rctWndFrame.c.top>ptMouse.c.v)
						BarMove(RIGHTBAR,-1,P_SMOOTH,aidWnd,TRUE);
					if(rctWndFrame.c.bottom<ptMouse.c.v)
						BarMove(RIGHTBAR,1,P_SMOOTH,aidWnd,TRUE);
				}
			}else
				lScrollTimer=0;

			/* ウインドウの作業領域を表示長方形にする */
			wget_wrk(aidWnd,&rctWndFrame);
			gset_vis(idDrawEnv,rctWndFrame);
		}

		DrawSelectFrame(-idDrawEnv,1,ptMouse.c.h-aptStart.c.h,
						ptMouse.c.v-aptStart.c.v); /* 枠の表示 */
		ptOldMouse=ptMouse;
	}

	DrawSelectFrame(-idDrawEnv,0,ptOldMouse.c.h-aptStart.c.h,
					ptOldMouse.c.v-aptStart.c.v); /* 枠の消去 */
	wend_drg(); /* ドラッグの終了 */
	FreeSelectFrame();

	if(isBreak)
		return; /* ドラッグが中断されたなら何もせずに終了 */
	if(aptStart.l==ptMouse.l)
		return;

	if(evDragEvent.s.stat & (ES_BUT2|ES_CMD)) /* ボタン2が押されていた */
		isCopy=TRUE;
	else
		isCopy=FALSE;
	if(evDragEvent.s.wid==aidWnd) /* 元のウインドウなら */
	{
		if(aisNewObject)
			MakeNewObject(aidWnd,ptMouse.c.h-aptStart.c.h,
								ptMouse.c.v-aptStart.c.v,0);
		else if(isCopy)
			CopySelectedData(aidWnd,ptMouse.c.h-aptStart.c.h,
								ptMouse.c.v-aptStart.c.v);
		else
			MoveSelectedData(aidWnd,ptMouse.c.h-aptStart.c.h
								,ptMouse.c.v-aptStart.c.v);
	}else{ /* 他のウインドウなら */
		WEVENT evPaste;
		POINT ptCursorMove; /* 正しい位置に貼り込まれるための修正値 */

		if(aisNewObject)
			MakeNewObject(aidWnd,0,0,evDragEvent.s.wid); /* 仮登録 */
		ptCursorMove=SendSelectedDataToTray(TRUE,aptStart);
		evPaste.r.type=EV_REQUEST;
		evPaste.r.cmd=W_PASTE;
		evPaste.r.wid=evDragEvent.s.wid;
		evPaste.s.pos=evDragEvent.s.pos;
		evPaste.s.pos.c.h+=ptCursorMove.c.h;
		evPaste.s.pos.c.v+=ptCursorMove.c.v;
		evPaste.s.time=evDragEvent.s.time;
		wsnd_evt(&evPaste); /* 貼り込み要求をだす */
		if(wwai_rsp((WEVENT*)NULL,W_PASTE,10000)!=W_ACK)
		{
			ErrPanel(ID_ST_NPASTE,FALSE);
			return;
		}
		wswi_wnd(evDragEvent.s.wid,(WEVENT*)NULL); /* 貼り込み先のウインドウに
														フォーカスを移す */
		if(aisNewObject)
			DeleteSelectedData(aidWnd,TRUE); /* 他のウインドウに貼り込むために
												仮登録されたデータを消す */
		else if(!isCopy)
			DeleteSelectedData(aidWnd,FALSE);
	}
}

/* 変形用の選択枠の表示 */
void DrawResizeFrame(aidDraw,apsrResizeFrame,arctOriginal,awGripPoint,
					aptStart,aptCurrent,awMode)
WORD aidDraw;
SEL_RGN* apsrResizeFrame;
RECT arctOriginal;
WORD awGripPoint;
POINT aptStart,aptCurrent;
WORD awMode;
{
	WORD wTmp;

	apsrResizeFrame->rgn.r=arctOriginal;
	if(awGripPoint==V_LTHD || awGripPoint==V_LBHD)
		apsrResizeFrame->rgn.r.c.left+=aptCurrent.c.h-aptStart.c.h;
	if(awGripPoint==V_RTHD || awGripPoint==V_RBHD)
		apsrResizeFrame->rgn.r.c.right+=aptCurrent.c.h-aptStart.c.h;
	if(awGripPoint==V_LTHD || awGripPoint==V_RTHD)
		apsrResizeFrame->rgn.r.c.top+=aptCurrent.c.v-aptStart.c.v;
	if(awGripPoint==V_LBHD || awGripPoint==V_RBHD)
		apsrResizeFrame->rgn.r.c.bottom+=aptCurrent.c.v-aptStart.c.v;

	if(apsrResizeFrame->rgn.r.c.right<apsrResizeFrame->rgn.r.c.left)
	{
		wTmp=apsrResizeFrame->rgn.r.c.left;
		apsrResizeFrame->rgn.r.c.left=apsrResizeFrame->rgn.r.c.right;
		apsrResizeFrame->rgn.r.c.right=wTmp;
	}
	if(apsrResizeFrame->rgn.r.c.bottom<apsrResizeFrame->rgn.r.c.top)
	{
		wTmp=apsrResizeFrame->rgn.r.c.top;
		apsrResizeFrame->rgn.r.c.top=apsrResizeFrame->rgn.r.c.bottom;
		apsrResizeFrame->rgn.r.c.bottom=wTmp;
	}
	adsp_sel(aidDraw,apsrResizeFrame,awMode);
}


/* 矩形を変形 */
RECT DragResizeRectFrame(aidWnd,aptStart,arctOriginal,awGripPoint)
WORD aidWnd;
POINT aptStart;
RECT arctOriginal; /* 変形させたい矩形 */
WORD awGripPoint; /* サイズを変更する場所 V_LTHD V_RTHD V_LBHD V_RBHD */
{
	WORD idDrawEnv; /* ドラッグ描画環境 */
	SEL_RGN srResizeFrame;
	POINT ptMouse,ptOldMouse;
	WEVENT evDragEvent; /* ドラッグ中のイベント */
	BOOL isBreak; /* ドラッグが中断されたかどうか */

	srResizeFrame.sts=0x0000;
	srResizeFrame.rgn.r=arctOriginal;
	isBreak=FALSE;
	ptOldMouse=aptStart;
	idDrawEnv=wsta_drg(aidWnd,0); /* ドラッグの開始(lock なし) */
	DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
					aptStart,aptStart,1); /* 枠の表示 */

	while(wget_drg(&ptMouse,&evDragEvent)!=EV_BUTUP)
	{
		POINT ptAbsolute; /* マウスのグローバル座標 */
		RECT rctWndFrame;

		if(evDragEvent.s.type==EV_KEYDWN &&
			evDragEvent.e.data.key.code==TK_CAN) /* ×キーが押されたとき */
		{
			isBreak=TRUE;
			break; /* ドラッグの中断 */
		}
		if(ptMouse.l==ptOldMouse.l)
		{
			ptOldMouse=ptMouse;
			continue;
		}
		DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
						aptStart,ptOldMouse,0); /* 枠の消去 */

		/* スクロール */
		wget_wrk(aidWnd,&rctWndFrame);
		if(rctWndFrame.c.left>ptMouse.c.h)
			BarMove(BOTTOMBAR,-1,P_SMOOTH,aidWnd,TRUE);
		if(rctWndFrame.c.right<ptMouse.c.h)
			BarMove(BOTTOMBAR,1,P_SMOOTH,aidWnd,TRUE);
		if(rctWndFrame.c.top>ptMouse.c.v)
			BarMove(RIGHTBAR,-1,P_SMOOTH,aidWnd,TRUE);
		if(rctWndFrame.c.bottom<ptMouse.c.v)
			BarMove(RIGHTBAR,1,P_SMOOTH,aidWnd,TRUE);

		DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
						aptStart,ptMouse,1); /* 枠の表示 */
		ptOldMouse=ptMouse;
	}

	DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
					aptStart,ptOldMouse,0); /* 枠の消去 */
	wend_drg(); /* ドラッグの終了 */

	if(isBreak) /* ドラッグが中断された */
		return arctOriginal;

	if(awGripPoint==V_LTHD || awGripPoint==V_LBHD)
		arctOriginal.c.left+=ptMouse.c.h-aptStart.c.h;
	if(awGripPoint==V_RTHD || awGripPoint==V_RBHD)
		arctOriginal.c.right+=ptMouse.c.h-aptStart.c.h;
	if(awGripPoint==V_LTHD || awGripPoint==V_RTHD)
		arctOriginal.c.top+=ptMouse.c.v-aptStart.c.v;
	if(awGripPoint==V_LBHD || awGripPoint==V_RBHD)
		arctOriginal.c.bottom+=ptMouse.c.v-aptStart.c.v;

	if(arctOriginal.c.right<arctOriginal.c.left)
	{
		WORD wTmp;

		wTmp=arctOriginal.c.left;
		arctOriginal.c.left=arctOriginal.c.right;
		arctOriginal.c.right=wTmp;
	}
	if(arctOriginal.c.bottom<arctOriginal.c.top)
	{
		WORD wTmp;

		wTmp=arctOriginal.c.top;
		arctOriginal.c.top=arctOriginal.c.bottom;
		arctOriginal.c.bottom=wTmp;
	}

	return arctOriginal;
}


/* 仮身のドラッグ変形 */
void DragResize(aidWnd,aptStart,adiVobj,awGripPoint)
WORD aidWnd;
POINT aptStart;
DOCITERATOR adiVobj;
WORD awGripPoint; /* サイズを変更する場所 V_LTHD V_RTHD V_LBHD V_RBHD */
{
	RECT rctOriginal;
	RECT rctResult;

	if(IsFixed(adiVobj))
		return;

	/* ポインタの形状の変更 */
	gset_ptr(wPointStile[1][awGripPoint],(PTRIMAGE*)NULL,-1L,-1L);

	rctOriginal=GetRect(adiVobj);
	rctResult=DragResizeRectFrame(aidWnd,aptStart,rctOriginal,awGripPoint);
	if(rctResult.l[0]==rctOriginal.l[0] && rctResult.l[1]==rctOriginal.l[1])
		return;

	if(orsz_vob(GetVID(adiVobj),&rctResult,V_SIZE)==0)
	{
		DrawOneRect(aidWnd,rctOriginal);
		ChangeRect(adiVobj,rctResult);
		DrawOneRect(aidWnd,rctResult);
	}
}


/* ドラッグ枠で選択 */
void DragSelect(aidWnd,aptStart)
WORD aidWnd;
POINT aptStart;
{
	RECT rctOriginal;
	RECT rctResult;
	DOCITERATOR diCurrent;

	rctOriginal.p.lefttop=aptStart;
	rctOriginal.p.rightbot=aptStart;
	/* 枠のドラッグ */
	rctResult=DragResizeRectFrame(aidWnd,aptStart,rctOriginal,V_RBHD);

	diCurrent=GetNext(NULL);
	while(diCurrent!=NULL)
	{
		RECT rctData;

		rctData=GetRect(diCurrent);
		if( rctData.c.left>rctResult.c.left &&
			rctData.c.top>rctResult.c.top &&
			rctData.c.right<rctResult.c.right &&
			rctData.c.bottom<rctResult.c.bottom ) /* 枠に入っているなら */
		{
			SelectVobj(diCurrent);
		}
		diCurrent=GetNext(diCurrent);
	}
}


/* 作業領域でマウスのボタンが押されたときの処理 */
void PushWorkArea(apevWnd)
WEVENT* apevWnd;
{
	WORD idVobj;
	WORD wLocation;
	DOCITERATOR diPointed;

	/* 化身の位置の検索 */
	diPointed=FindVobj(apevWnd->s.pos,&wLocation);
	if(diPointed==NULL) /* 仮身を指していない場合 */
	{
		DrawSelectFrame(apevWnd->s.wid,0,0,0);
		FreeSelectFrame();
		if(!(apevWnd->s.stat&ES_LSHFT)||(apevWnd->s.stat&ES_RSHFT))
			UnSelectAllVobj();
		if(wchk_dck(apevWnd->s.time)==W_PRESS)
			DragSelect(apevWnd->s.wid,apevWnd->s.pos); /* ドラッグ枠で選択 */
	}else{
		WORD wClick;

		/* ポインタの形状の変更 */
		gset_ptr(wPointStile[0][wLocation],(PTRIMAGE*)NULL,-1L,-1L);

		DrawSelectFrame(apevWnd->s.wid,0,0,0); /* 選択枠を消去 */
		FreeSelectFrame(); /* 選択枠リストのメモリを解放 */

		if(IsBackground(diPointed)) /* 背景化されている場合 */
		{ /* 特別扱いする */
			if( (wLocation==V_PICT)&&(wchk_dck(apevWnd->s.time)==W_DCLICK) )
				oexe_apg(GetVID(diPointed),0); /* デフォルトアプリの実行 */
			else
				UnSelectAllVobj();
			MakeSelectFrame(); /* 選択枠リストを作成 */
			DrawSelectFrame(apevWnd->s.wid,1,0,0); /* 選択枠を表示 */
			return;
		}

		if((apevWnd->s.stat&ES_LSHFT)||(apevWnd->s.stat&ES_RSHFT))
		{ /* シフトが押されていたとき */
			SwitchVobjSelect(diPointed); /* 選択状況を反転する */
			MakeSelectFrame(); /* 選択枠リストを作成 */
			DrawSelectFrame(apevWnd->s.wid,1,0,0); /* 選択枠を表示 */
			return;
		}

		if(IsSelected(diPointed)==FALSE)
		{ /* 選択されていなかった物を押したとき */
			UnSelectAllVobj();
			SwitchVobjSelect(diPointed); /* 選択されたことにする */
		}

		wClick=wchk_dck(apevWnd->s.time);
		switch(wLocation)
		{
		case V_NAME:
			if(wClick==W_DCLICK)
			{ /* 実身名の変更 */
				if(ochg_nam(GetVID(diPointed),(TPTR)NULL)==1)
					DrawOneRect(apevWnd->s.wid,GetRect(diPointed));
				break;
			}
		case V_RELN:
			if(wClick==W_DCLICK)
			{ /* 続柄の変更 */
				if(ochg_rel(GetVID(diPointed),-1)==1)
				{
					ChangedVirtualObjRelation(diPointed);
					DrawOneRect(apevWnd->s.wid,GetRect(diPointed));
				}
				break;
			}
		case V_PICT:
			if(wClick==W_DCLICK)
			{
				oexe_apg(GetVID(diPointed),0); /* デフォルトアプリの実行 */
				break;
			}
		case V_WORK:
		case V_FRAM:
			switch(wClick)
			{
			case W_PRESS:
				/* ドラッグ移動 */
				DragMove(apevWnd->s.wid,apevWnd->s.pos,FALSE);
				break;
			case W_QPRESS:
				/* 新版作成 */
				DragMove(apevWnd->s.wid,apevWnd->s.pos,TRUE);
				break;
			}
			break;
		case V_LTHD:
		case V_RTHD:
		case V_LBHD:
		case V_RBHD:
			if(wClick==W_PRESS)
			{
				/* ドラッグ変形 */
				DragResize(apevWnd->s.wid,apevWnd->s.pos,diPointed,wLocation);
			}
			break;
		}
	}
	MakeSelectFrame(); /* 選択枠リストを作成 */
	DrawSelectFrame(apevWnd->s.wid,1,0,0); /* 選択枠を表示 */
}

void ExecSelectedVobj()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		oexe_apg(GetVID(diCurrent),0); /* デフォルトアプリの実行 */
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
}

void ChangeForcusToParentWindow(aidWnd)
WORD aidWnd;
{
	WDSTAT wsStatus;

	wget_sts(aidWnd,&wsStatus,(WDDISP*)NULL);
	if(wsStatus.parent!=0)
		wswi_wnd(wsStatus.parent,(WEVENT*)NULL);
}

void SelectedVobjInfo()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		odsp_inf(GetVID(diCurrent));
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
}

/* キーを押したときの処理 */
BOOL KeyDown(aevWnd,aidWnd)
WEVENT aevWnd;
WORD aidWnd;
{
	static KEYTAB* pktKey=NULL;
	TCODE tcAlphaKey;

	if(pktKey==NULL)
	{ /*キーボード表の取り出し */
		WORD wSize;

		wSize=get_ktb((KEYTAB*)NULL);
		get_lmb(&pktKey,(LONG)wSize,NOCLR);
		get_ktb(pktKey);
	}
	tcAlphaKey=pktKey->kct[pktKey->key_max*pktKey->kct_sel[0x01]+
							aevWnd.e.data.key.keytop];
	if(aevWnd.s.stat & ES_CMD) /* 命令キーが押されている */
	{
		switch(aevWnd.e.data.key.code)
		{
		/* カーソルキー */
		case KEY_UP:
			BarMove(RIGHTBAR,-1,P_AREA,aidWnd,FALSE);
			break;
		case KEY_DOWN:
			BarMove(RIGHTBAR,1,P_AREA,aidWnd,FALSE);
			break;
		case KEY_RIGHT:
			BarMove(BOTTOMBAR,1,P_AREA,aidWnd,FALSE);
			break;
		case KEY_LEFT:
			BarMove(BOTTOMBAR,-1,P_AREA,aidWnd,FALSE);
			break;
		default:
			return MenuKey(aevWnd.s.wid,aidWnd,aevWnd.s.pos);
		}
	}else if(aevWnd.s.stat & (ES_LSHFT | ES_RSHFT)) /* シフトが押されている */
	{
		switch(aevWnd.e.data.key.code)
		{
		case TK_BS: /* バックスペース */
			return FALSE; /* 終了 */
		case 0x000D: /* リターンキー */
			ExecSelectedVobj(); /* 実行 */
			return FALSE; /* 終了 */
		}
	}else{
		switch(tcAlphaKey/*aevWnd.e.data.key.code*/)
		{
		case TK_DEL: /* デリートキー */
			DrawSelectFrame(aidWnd,0,0,0); /* 選択枠を消去 */
			FreeSelectFrame(); /* 選択枠リストのメモリを解放 */
			DeleteSelectedData(aidWnd,FALSE); /* 削除 */
			break;
		case TK_NL: /* リターンキー */
			ExecSelectedVobj(); /* 実行 */
			break;
		case TK_BS: /* バックスペース */
			ChangeForcusToParentWindow(aidWnd);
							/* 親ウインドウにフォーカスを移す */
			break;
		case TK_i:
			SelectedVobjInfo(); /* 実身管理情報の表示 */
			break;
		}
	}

	return TRUE;
}


/* 全面表示切り替え */
void SwitchFullWindow(aidWnd)
WORD aidWnd;
{
	/* インジケータの変更 */
	SwitchFullWindowIndicator();

	/* 全面表示切り替え */
	if(wchg_wnd(aidWnd,NULL,W_MOVE)>0)
		DrawView(aidWnd);
}

void NullEvent(aevWnd,aidWnd)
WEVENT aevWnd;
WORD aidWnd;
{
	WORD wNewPointStile;

	DrawSelectFrame(aidWnd,-1,0,0);
	if(aevWnd.s.wid!=aidWnd) /* 他のウインドウ上なら何もしない */
		return;
	if(aevWnd.s.stat&ES_CMD) /* 命令キーが押されているとき */
		wNewPointStile=PS_MENU;
	else if(aevWnd.s.cmd!=W_WORK) /* 作業領域以外の時 */
		return;
	else if((aevWnd.s.stat&ES_LSHFT)||(aevWnd.s.stat&ES_RSHFT))
		/* シフトが押されているとき */
		wNewPointStile=PS_MODIFY;
	else /* 作業領域の時 */
	{
		DOCITERATOR diPointed;
		WORD wLocation;

		/* 化身の位置の検索 */
		diPointed=FindVobj(aevWnd.s.pos,&wLocation);
		if(diPointed!=NULL)
		{ /* ポインタの形状の変更 */
			if(IsSelected(diPointed)==TRUE)
				wNewPointStile=wPointStile[0][wLocation];
			else
				wNewPointStile=PS_SELECT;
		}else
			wNewPointStile=PS_SELECT;
	}
	gset_ptr(wNewPointStile,(PTRIMAGE*)NULL,-1L,-1L);
}

void BackgroundSelectedVobj(aidWnd)
WORD aidWnd;
{
	DOCITERATOR diCurrent;

	TakeFrontOrBackSelectedData(FALSE);/*背景化されたら一番最初に持ってくる */
	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		BackgroundVobj(diCurrent); /* 背景化 */
		DrawOneRect(aidWnd,GetRect(diCurrent));
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
}

/* プログラムが終了する前にファイルのセーブの処理をする */
BOOL EndProg()
{
	if(IsDataModified())
	{
		gset_ptr(PS_SELECT,(PTRIMAGE*)NULL,-1L,-1L);
		switch(pact_err(ID_PN_SAVE,(TPTR)NULL,(TPTR)NULL,(TPTR)NULL))
		{
		case ID_PN_SAVE_OK:
			if(!SaveFile())
				return TRUE; /* 保存に失敗したなら保存しない */
			return FALSE; /* 終了 */
		case ID_PN_SAVE_NO:
			return FALSE; /* 終了 */
		case ID_PN_SAVE_CANCEL:
			return TRUE;
		}
	}else if(IsSegmentOnlyModified())
		SaveFile(); /* こっそり保存 */
	return FALSE;
}


/* エラーパネルの表示 */
void ErrPanel(awMessage,aisDouble)
WORD awMessage; /* 表示するストリングを示すデータボックスの番号 */
				/* メモリが足りないなら 0 を指定する */
BOOL aisDouble; /* 複数行=TRUE 1行=FALSE */
{
WORD err;
	gset_ptr(PS_SELECT,(PTRIMAGE*)NULL,-1L,-1L);
	if(awMessage==0)
	{
		err=pact_err(ID_PN_ERR1,NULL,NULL,NULL);
	}else{
		if(aisDouble)
		{
			WORD key[2];
			TPTR ptMessage[2];

			key[0]=dget_dat(TEXT_DATA,awMessage,0);
			key[1]=dget_dat(TEXT_DATA,awMessage+1,0);
			smb_adr(key[0],&(ptMessage[0]));
			smb_adr(key[1],&(ptMessage[1]));
			err=pact_err(ID_PN_ERR1,"\0",ptMessage[0],ptMessage[1]);
			drel_dat(key[0]);
			drel_dat(key[1]);
		}else{
			WORD key;
			TPTR ptMessage;

			key=dget_dat(TEXT_DATA,awMessage,0);
			smb_adr(key,&ptMessage);
			err=pact_err(ID_PN_ERR1,ptMessage,NULL,NULL);
			drel_dat(key);
		}
	}
	return;
}
