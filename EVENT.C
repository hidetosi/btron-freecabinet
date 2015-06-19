#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"


/* マウスのボタンを押したときの処理 */
BOOL MouseButtonDown(aidWnd,aevWnd)
WORD aidWnd;
WEVENT *aevWnd;
{
	if(aidWnd!=aevWnd->s.wid)
		return TRUE;
	switch(aevWnd->s.cmd)
	{
	case W_LTHD: /* ハンドル */
	case W_RTHD:
	case W_LBHD:
	case W_RBHD:
		switch(wchk_dck(aevWnd->s.time))
		{
		case W_DCLICK:
			SwitchFullWindow(aidWnd); /* 全面表示切り替え */
			SetBarValue(aidWnd);
			break;

		case W_PRESS:
		case W_QPRESS:
			/* ドラッグ変形する */
			if(wrsz_drg(aevWnd,NULL,NULL)==1)
				DrawView(aevWnd->s.wid);
			if(GetFullWindowIndicator()==TRUE)
				SwitchFullWindowIndicator();
			SetBarValue(aidWnd);
			break;
		}
		break;

	case W_PICT: /* ピクトグラム */
		switch (wchk_dck(aevWnd->s.time))
		{
		case W_DCLICK: /* ダブルクリックの時 */
			return FALSE; /* 終了 */
		case W_PRESS:
			break; /* 移動 */
		default:
			return TRUE;
		}

	case W_FRAM: /* 枠 */
	case W_TITL: /* タイトルバー */
		/* ドラッグ移動する */
		if(wmov_drg(aevWnd, NULL)==1)
			DrawView(aevWnd->s.wid);
		if(GetFullWindowIndicator()==TRUE)
			SwitchFullWindowIndicator();
		break;

	case W_RBAR: /* 右スクロールバー */
		ActScrollBar(RIGHTBAR,aevWnd);
		break;
		
	case W_BBAR: /* 下スクロールバー */
		ActScrollBar(BOTTOMBAR,aevWnd);
		break;

	case W_WORK:
		PushWorkArea(aevWnd);
		break;
	}
	return TRUE;
}


BOOL EventExec(aevWnd,aidWnd)
WEVENT aevWnd;
int aidWnd;
{
	switch (aevWnd.s.type)
	{
	case EV_NULL:
		NullEvent(aevWnd,aidWnd);
		break;
	case EV_REQUEST:
		switch(aevWnd.g.cmd)
		{
		case W_REDISP:/* 再表示要求 */
			DrawView(aidWnd);
			break;
		case W_PASTE: /* 貼込み要求*/
			DrawSelectFrame(aidWnd,0,0,0); /* 選択枠の消去 */
			FreeSelectFrame();
			PasteFromTray(aidWnd,TRUE,aevWnd.s.pos);
			wrsp_evt(&aevWnd,0);
			MakeSelectFrame();
			SetBarValue(aidWnd);
			break;
		case W_DELETE: /* ウインドウのクローズ */
		case W_FINISH: /* 処理終了 */
			ForceNoSave();
			wrsp_evt(&aevWnd,0);
			return FALSE;
		case W_VOBJREQ: /* 仮身要求イベント */
			switch(aevWnd.g.data[2])
			{
			case 0: /* 名称の変更 */
			case 1: /* 処理中状態になった */
			case 2: /* 処理中状態が解除された */
			case 3: /* 虚身状態になった */
			case 4: /* 虚身状態が解除された */
/* b_printf("request=%d ID=%d\n",aevWnd.g.data[2],aevWnd.g.data[3]); */
				{ /* なぜか身に覚えのない仮身ID(ID=1)の要求が来るので
				 		その対策をしてあります。もしかして 1B のバグ？ */
					DOCITERATOR diChange;
					if((diChange=GetIterator(aevWnd.g.data[3]))!=NULL)
						DrawOneRect(aidWnd,GetRect(diChange));
												/* 仮身の再描画 */
				}
				break;
			case 5: /* 虚身ウインドウになった */
			case 6: /* 虚身ウインドウが解除になった */
				wreq_dsp(aidWnd); /* 再描画 */
				break;
			case 15: /* 仮身の変更通知 */
				VobjSegmentModified(aevWnd.g.data[3]); /* 変更のチェック */
				break;
			case 16: /* 仮身の挿入要求 */
			case 17: /* 仮身の挿入要求 */
				DrawSelectFrame(aidWnd,0,0,0);
				AddVobjByID(aevWnd.g.data[3]); /* 仮身の追加 */
				DrawOneVobj(aevWnd.g.data[3],aidWnd); /* 追加した仮身の描画 */
				SetBarValue(aidWnd);
				break;
			case 128: /* 一時ファイルへの格納要求 */
				break;
			}
			break;
		}
		break;
	case EV_RSWITCH:
		DrawView(aidWnd);
	case EV_SWITCH:
	case EV_BUTDWN: /* マウスのボタンが押されたとき */
		return MouseButtonDown(aidWnd,&aevWnd);
	case EV_MENU: 
		return OpenMenu(aevWnd.s.pos,aidWnd);
	case EV_AUTKEY:
	case EV_KEYDWN: /* キーが押されたとき */
		return KeyDown(aevWnd,aidWnd);
	case EV_INACT:
		DrawSelectFrame(aidWnd,0,0,0); /* 選択枠の消去 */
		break;
	case EV_DEVICE:
		oprc_dev(&aevWnd,NULL,0);
		break;
	case EV_MSG:
		{ /* メッセージは捨てる */
			WORD aMsg[32];
			rcv_msg(0x7fffffffL,aMsg,sizeof(aMsg),NOWAIT|CLR);
			break;
		}
	}
	return TRUE;
}
