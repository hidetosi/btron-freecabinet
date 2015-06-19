#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"

void ErrEnd(aErrMsg)
TPTR aErrMsg;
{
	pdsp_msg(aErrMsg);
	ext_prc(0);
}

WORD OpenWindow(apMsg,apwUniqueData,aisDisp)
M_EXECREQ* apMsg;
WORD* apwUniqueData; /* 固有データ */
BOOL aisDisp; /* 開いた仮身かどうか */
{
	WORD idWnd; /* ウィンドウID */
	RECT rctWnd,rctWork; /*ウィンドウの表示位置*/
	TCODE tFilename[21]; /* 実身名 */
	PATTERN ptnBack; /* 背景パターン */

	ptnBack.spat.kind=0;
	ptnBack.spat.hsize=8;
	ptnBack.spat.vsize=1;
	ptnBack.spat.bgcol=-1;
	ptnBack.spat.mask=FILL100;

	/* 固有データの読み込み */
	if(!ReadUniqueData(apwUniqueData,&rctWnd,&rctWork))
	{
		rctWnd.c.left=100;
		rctWnd.c.top=100;
		rctWnd.c.right=300;
		rctWnd.c.bottom=300;
		rctWork.c.left=0;
		rctWork.c.top=0;
		rctWork.c.right=0;
		rctWork.c.bottom=0;
	}

	if(aisDisp) /* 開いた仮身なら */
	{
		M_DISPREQ* pDispReq;
		WORD idDrawEnv;

		pDispReq=(M_DISPREQ*)apMsg;
		ptnBack.spat.fgcol=pDispReq->bgcol;

		idWnd=wopn_iwd(pDispReq->gid);
		idDrawEnv=wget_gid(idWnd);
		gmov_cor(idDrawEnv,rctWork.c.left,rctWork.c.top);
		gget_fra(idDrawEnv,&rctWnd);
		gset_vis(idDrawEnv,rctWnd);
		gfil_rec(idDrawEnv,rctWnd,&ptnBack,0,G_STORE);
		return -idWnd;
	}else{ /* 仮身のオープンなら */
		ptnBack.spat.fgcol=apMsg->bgcol;

		fil_sts(&(apMsg->lnk),tFilename,NULL,NULL); /* 実身名の獲得 */

		/*ウィンドウをオープン*/
		idWnd = wopn_wnd(WA_SIZE|WA_HHDL|WA_VHDL|WA_RBAR|WA_BBAR,
					apMsg->pwid,&rctWnd,NULL,6,tFilename,&ptnBack,NULL);
		if (idWnd < 0)
			ErrEnd(L"ウインドウが開けませんでした");
		wset_wrk(idWnd,&rctWork); /* データ内の表示する位置を設定 */
		return idWnd;
	}

}

void MAIN(apMsg)
M_EXECREQ* apMsg;
{
	WORD idWnd; /* ウィンドウID */
	WORD idDatabox; /* データボックスのファイルID */
	WORD* pwUniqueData; /* 固有データ */
	WDSTAT wdsWindowState; /* ウインドウの状態 */

	gset_ptr(PS_BUSY,NULL,-1L,-1L); /* ポインタをゆのみにする */

	chg_wrk(NULL);

	idDatabox=dopn_dat(&(apMsg->self)); /* データボックスのオープン */
	if( idDatabox < 0)
		ErrEnd(L"データボックスが開けませんでした");

	/* 起動の仕方による動作の違い */
	switch(apMsg->type)
	{
	case EXECREQ:
		if(!(apMsg->mode & 0x0002)) /* 仮身起動の時 */
		{
			if(apMsg->mode & 0x0001) /* デフォルト起動でない時 */
				pwUniqueData=NULL;
			else /* デフォルト起動の時 */
				pwUniqueData=apMsg->info;

			if(!FileOpen(apMsg->vid,apMsg->lnk))
				ErrEnd("\0"); /* ファイルが開けなかった */
			idWnd=OpenWindow(apMsg,pwUniqueData,(BOOL)FALSE);
			osta_prc(apMsg->vid,idWnd); /* 実身処理の開始 */
			ReadFile(idWnd); /* 実身の読み込み */

			break; /* 次に進む */
		}
	case DISPREQ:
		pwUniqueData=apMsg->info;

		if(!FileOpen(apMsg->vid,apMsg->lnk))
			ErrEnd("\0"); /* ファイルが開けなかった */
		idWnd=OpenWindow(apMsg,pwUniqueData,(BOOL)TRUE);

		ReadFile(idWnd); /* 実身の読み込み */

		oend_req(apMsg->vid,0);
		ext_prc(0); /* 終了 */

	case PASTEREQ:
		oend_req(apMsg->vid,1);
	default:
		ErrEnd(L"実行の仕方が不正です");
	}

	MenuInit(); /* メニューの初期化 */

	InitScrollBar(idWnd); /* スクロールバーの初期化 */

	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* ポインタを元に戻す */

	for (;;)  /*イベントループ*/
	{
		WEVENT evWnd; /* イベント */

		wget_evt(&evWnd, WAIT);	/*イベントを取得*/
		if(!EventExec(evWnd,idWnd))
		{
			if(!EndProg())
				break; /* 終了 */
		}
	}
	dcls_dat(idDatabox); /* データボックスのクローズ */

	/* プログラムの終了処理 */
	gset_ptr(PS_BUSY, NULL, -1L, -1L); /* ポインタを変更 */
	wget_sts(idWnd,&wdsWindowState,NULL); /* ウインドウの状態の取得 */
	WriteUniqueData(wdsWindowState.r,wdsWindowState.wr);
	wcls_wnd(idWnd, CLR); /* ウインドウのクローズ */
	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* ポインタを元に戻す */
	ext_prc(0);
}
