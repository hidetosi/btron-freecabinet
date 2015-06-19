#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"

WORD idMenu;
MENUITEM itmMNExec,itmMNApplet,itmMNVobj,itmMNObj,itmMNFusen,
			itmMNDisk,itmMNWnd;

/* メニューの初期化 */
BOOL MenuInit()
{
	idMenu=mopn_men(ID_MN); /* メニューの登録 */
	if(idMenu<0)
	{
		return FALSE;
	}

	/* メニュー項目の取り出し */
	mget_itm(idMenu,ID_MN_EXEC,&itmMNExec);
	mget_itm(idMenu,ID_MN_APPLET,&itmMNApplet);
	mget_itm(idMenu,ID_MN_VOBJ,&itmMNVobj);
	mget_itm(idMenu,ID_MN_OBJ,&itmMNObj);
	mget_itm(idMenu,ID_MN_FUSEN,&itmMNFusen);
	mget_itm(idMenu,ID_MN_DISK,&itmMNDisk);
	mget_itm(idMenu,ID_MN_WND,&itmMNWnd);

	return TRUE;
}


void ExecVobjMenu(awSelectedMenu,aidWnd)
UWORD awSelectedMenu;
WORD aidWnd;
{
	int iSelectedNum;

	iSelectedNum=GetSelectedVobjNum();
	 /* （選択された化身がないときは来ないはず） */
	if(iSelectedNum==1)
	{
		WORD item;
		RECT buf;
		RECT rctOld;
		WORD wOperateType;
		DOCITERATOR diCurrent;
		WORD idCurrentVobj;

		if((awSelectedMenu >> 8) == ID_MN_VOBJ)
			item=0;
		else if((awSelectedMenu >> 8) == ID_MN_OBJ)
			item=0x100;
		else
			item=0x200;
		if((awSelectedMenu >> 8) == ID_MN_FUSEN)
			item=(0x100 + (awSelectedMenu & 0x00ff));
		else
			item=-(item + (awSelectedMenu & 0x00ff));

		/* 化身・実身・ディスク操作の実行 */
		diCurrent=GetNextSelectedVobj(NULL);
		idCurrentVobj=GetVID(diCurrent);
		wOperateType=oexe_vmn(idCurrentVobj,item,&buf);
		switch(wOperateType)
		{
		case 2: /* VM_CLOSE */
			rctOld=GetRect(diCurrent);
			ChangeRect(diCurrent,buf);
			DrawOneRect(aidWnd,rctOld);
			SetBarValue(aidWnd);
			break;
		case VM_OPEN:
			ChangeRect(diCurrent,buf);
			DrawOneRect(aidWnd,GetRect(diCurrent));
			SetBarValue(aidWnd);
			break;
		case VM_DISP:
			rctOld=GetRect(diCurrent);
			ChangeRect(diCurrent,buf);
			DrawOneRect(aidWnd,rctOld);
			DrawOneRect(aidWnd,GetRect(diCurrent));
			break;
		case VM_NEW:
			AddVobjByID(((WORD*)&buf)[0]);
			DrawOneVobj(((WORD*)&buf)[0],aidWnd);
			SetBarValue(aidWnd);
			break;
		case VM_RELN:
			ChangedVirtualObjRelation(diCurrent);
		case VM_NAME:
		case VM_DETACH:
		case VM_REFMT:
		case VM_PASTE:
		case VM_ATTACH:
			DrawOneRect(aidWnd,GetRect(diCurrent));
			break;
		}
	} else if(iSelectedNum>1)
	{
		WORD item;
		int i;
		WPTR pwBuf,p;
		WORD wOperateType;
		DOCITERATOR diCurrent;
		WORD idCurrentVobj;

		if(get_lmb(&pwBuf,(LONG)(iSelectedNum*(sizeof(RECT)+2)+2),NOCLR)<0)
		{
			ErrPanel(0,FALSE);
			return;
		}

		if((awSelectedMenu >> 8) == ID_MN_VOBJ)
			item=0;
		else if((awSelectedMenu >> 8) == ID_MN_OBJ)
			item=0x100;
		else
			item=0x200;
		if((awSelectedMenu >> 8) == ID_MN_FUSEN)
			item=(0x100 + (awSelectedMenu & 0x00ff));
		else
			item=-(item + (awSelectedMenu & 0x00ff));

		/* ID リストの作成 */
		diCurrent=GetNextSelectedVobj(NULL);
		for(i=0; diCurrent!=NULL; i++)
		{
			if(IsFixed(diCurrent)||IsBackground(diCurrent))
			{
				i--;
				diCurrent=GetNextSelectedVobj(diCurrent);
				continue;
			}
			pwBuf[i]=GetVID(diCurrent);
			diCurrent=GetNextSelectedVobj(diCurrent);
		}
		pwBuf[i]=0;

		/* 化身・実身・ディスク操作の実行 */
		wOperateType=oexe_vmn(-1,item,(BPTR)pwBuf);

		if(wOperateType==VM_NONE ||
		   (VM_EXREQ<=wOperateType && wOperateType<=VM_GABAGE))
			return;
		p=pwBuf;
		while(*p!=0)
		{
			RECT rctOld;
			diCurrent=GetIterator(*p);
			p++;

			switch(wOperateType)
			{
			case 2: /* VM_CLOSE */
				rctOld=GetRect(diCurrent);
				ChangeRect(diCurrent,*((RECT*)p));
				p+=4;
				DrawOneRect(aidWnd,rctOld);
				SetBarValue(aidWnd);
				break;
			case VM_OPEN:
				ChangeRect(diCurrent,*((RECT*)p));
				p+=4;
				DrawOneRect(aidWnd,GetRect(diCurrent));
				SetBarValue(aidWnd);
				break;
			case VM_DISP:
				rctOld=GetRect(diCurrent);
				ChangeRect(diCurrent,*((RECT*)p));
				p+=4;
				DrawOneRect(aidWnd,rctOld);
				DrawOneRect(aidWnd,GetRect(diCurrent));
				break;
			case VM_NEW:
				AddVobjByID(*p);
				DrawOneVobj(*p,aidWnd);
				p++;
				SetBarValue(aidWnd);
				break;
			case VM_RELN:
				ChangedVirtualObjRelation(diCurrent);
				p++;
			case VM_NAME:
			case VM_DETACH:
			case VM_REFMT:
			case VM_PASTE:
				DrawOneRect(aidWnd,GetRect(diCurrent));
				SetBarValue(aidWnd);
				break;
			}
		}
		rel_lmb(pwBuf);
	}
}

void TrayMenu(aidWnd,awType,aptPushed)
WORD aidWnd;
WORD awType;
POINT aptPushed;
{
	POINT ptLocalPoint;
	WORD idMouseWnd;

	wfnd_wnd(&aptPushed,&ptLocalPoint,&idMouseWnd);
	if(idMouseWnd!=aidWnd) /* メニューボタンを押した場所が
							他のウインドウの上だったとき */
	{
		ptLocalPoint.c.h=0;
		ptLocalPoint.c.v=0;
	}

	switch(awType)
	{
	case ID_MN_EDIT_COPY:
		/* トレイにコピー */
		if(GetSelectedVobjNum()>=1)
			SendSelectedDataToTray(FALSE,ptLocalPoint);
		break;
	case ID_MN_EDIT_CPPASTE:
		/* トレイからコピー */
		PasteFromTray(aidWnd,FALSE,ptLocalPoint);
		break;
	case ID_MN_EDIT_MOVE:
		/* トレイに移動 */
		if(GetSelectedVobjNum()>=1)
		{
			SendSelectedDataToTray(FALSE,ptLocalPoint);
			DeleteSelectedData(aidWnd);
		}
		break;
	case ID_MN_EDIT_MVPASTE:
		/* トレイから移動 */
		PasteFromTray(aidWnd,FALSE,ptLocalPoint);
		tdel_dat();
		break;
	}
}

/* メニューの結果に対する動作 */
/* 戻り値: 終了が選ばれた=FALSE, その他=TRUE */
BOOL ExecuteMenu(awSelectedMenu,aidWnd,aptPushed)
UWORD awSelectedMenu;
WORD aidWnd;
POINT aptPushed;
{
	DrawSelectFrame(aidWnd,0,0,0);
	FreeSelectFrame();

	if(awSelectedMenu<=0)
	{
		MakeSelectFrame();
		return TRUE;
	}
	switch(awSelectedMenu >>8)
	{
	case ID_MN_END:
		return FALSE;
	case ID_MN_SAVE:
		switch(awSelectedMenu & 0x00ff)
		{
		case ID_MN_SAVE_NEWFILE:
			SaveNewFile();
			break;
		case ID_MN_SAVE_OLDFILE:
			SaveFile();
			break;
		}
		break;
	case ID_MN_VIEW:
		switch(awSelectedMenu & 0x00ff)
		{
		case ID_MN_VIEW_FULLWND:
			/* 全面表示切り替え */
			SwitchFullWindow(aidWnd);
			break;
		case ID_MN_VIEW_REPAINT:
			/* 再描画 */
			wreq_dsp(aidWnd);
			break;
		}
		break;
	case ID_MN_EDIT:
		switch(awSelectedMenu & 0x00ff)
		{
		case ID_MN_EDIT_COPY:
		case ID_MN_EDIT_CPPASTE:
		case ID_MN_EDIT_MOVE:
		case ID_MN_EDIT_MVPASTE:
			TrayMenu(aidWnd,awSelectedMenu & 0x00ff,aptPushed);
			break;
		case ID_MN_EDIT_DELETE:
			/* 削除 */
			DeleteSelectedData(aidWnd,FALSE);
			break;
		case ID_MN_EDIT_ALL:
			SelectAllVobj();
			break;
		case ID_MN_EDIT_FRONT:
			TakeFrontOrBackSelectedData(TRUE);
			DrawSelectedData(aidWnd);
			break;
		case ID_MN_EDIT_BACK:
			TakeFrontOrBackSelectedData(FALSE);
			DrawSelectedData(aidWnd);
			break;
		}
		break;

	case ID_MN_PROTECT:
		switch(awSelectedMenu & 0x00ff)
		{
		case ID_MN_PROTECT_FIX: /* 固定化 */
			FixSelectedVobj();
			break;
		case ID_MN_PROTECT_UNFIX:
			NormalizeSelectedVobj();
			break;
		case ID_MN_PROTECT_BACK: /* 背景化 */
			BackgroundSelectedVobj();
			break;
		case ID_MN_PROTECT_UNBACK:
			UnBackgroundAllVobj();
			break;
		}
		break;

	case ID_MN_VOBJ:
	case ID_MN_OBJ:
	case ID_MN_FUSEN:
	case ID_MN_DISK:
		ExecVobjMenu(awSelectedMenu,aidWnd);
		break;
	case ID_MN_EXEC: /* アプリケーションの実行 */
		oexe_apg(GetVID(GetNextSelectedVobj(NULL)),awSelectedMenu);
		break;
	case ID_MN_WND:
		/* ウインドウの切り替え */
		wexe_dmn(awSelectedMenu);
		break;
	case ID_MN_APPLET:
		/* 小物の起動 */
		oexe_apg(0,awSelectedMenu);
		break;
	}
	MakeSelectFrame();
	return TRUE;
}

BOOL IsAllSelectedFixed()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		if(!IsFixed(diCurrent))
			return FALSE;
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
	return TRUE;
}

BOOL IsExistSelectedFixed()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		if(IsFixed(diCurrent))
			return TRUE;
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
	return FALSE;
}


BOOL IsExistBackground()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNext(NULL);
	while(diCurrent!=NULL)
	{
		if(IsBackground(diCurrent))
			return TRUE;
		diCurrent=GetNext(diCurrent);
	}
	return FALSE;
}

/* メニューの表示 */
/* 戻り値: 終了が選ばれた=FALSE, その他=TRUE */
BOOL OpenMenu(aptPoint,aidWnd)
POINT aptPoint;
WORD aidWnd;
{
	int VobjNum,CurrentVobj;
	UWORD SelectedMenu;

	VobjNum=GetSelectedVobjNum();
	if(VobjNum==0)
	{
		CurrentVobj=-1; /* 不能状態のメニューを取り出す */
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_COPY,M_INACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MOVE,M_INACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_DELETE,M_INACT);
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_BACK,M_INACT);
	}else if(VobjNum==1)
	{
		DOCITERATOR diCurrent;

		diCurrent=GetNextSelectedVobj(NULL);
		CurrentVobj=GetVID(diCurrent);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_COPY,M_ACT);
		if(IsFixed(diCurrent))
		{ /* 固定化されているとき */
			mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MOVE,M_INACT);
			mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_DELETE,M_INACT);
			mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_BACK,M_ACT);
		}else{
			mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MOVE,M_ACT);
			mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_DELETE,M_ACT);
			mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_BACK,M_ACT);
		}
	}else{
		CurrentVobj=0; /* 共通部分のみのメニューを取り出す */
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_COPY,M_ACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MOVE,M_ACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_DELETE,M_ACT);
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_BACK,M_ACT);
	}
	if(IsExistSelectedFixed())
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_UNFIX,M_ACT);
	else
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_UNFIX,M_INACT);
	if(IsAllSelectedFixed())
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_FIX,M_INACT);
	else
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_FIX,M_ACT);
	if(IsExistBackground())
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_UNBACK,M_ACT);
	else
		mchg_atr(idMenu,ID_MN_PROTECT<<8|ID_MN_PROTECT_UNBACK,M_INACT);

	if(tsel_dat(-1)==0) /* トレーが空のとき */
	{
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_CPPASTE,M_INACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MVPASTE,M_INACT);
	}else{
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_CPPASTE,M_ACT);
		mchg_atr(idMenu,ID_MN_EDIT<<8|ID_MN_EDIT_MVPASTE,M_ACT);
	}

	/* 実行・小物・化身操作・実身操作・ディスク操作のメニュー項目の設定 */
 	oget_vmn(CurrentVobj,&(itmMNExec.ptr),&(itmMNApplet.ptr),&(itmMNVobj.ptr),
 					&(itmMNObj.ptr),&(itmMNDisk.ptr),&(itmMNFusen.ptr));
	mset_itm(idMenu,ID_MN_EXEC,&itmMNExec);
	mset_itm(idMenu,ID_MN_APPLET,&itmMNApplet);
	mset_itm(idMenu,ID_MN_VOBJ,&itmMNVobj);
	mset_itm(idMenu,ID_MN_OBJ,&itmMNObj);
	mset_itm(idMenu,ID_MN_FUSEN,&itmMNFusen);
	mset_itm(idMenu,ID_MN_DISK,&itmMNDisk);
	if(VobjNum==1 && IsFixed(GetNextSelectedVobj(NULL)))
		mchg_atr(idMenu,ID_MN_VOBJ<<8,M_INACT);

	/* ウインドウのメニュー項目の設定 */
	wget_dmn(&itmMNWnd.ptr);
	mset_itm(idMenu,ID_MN_WND,&itmMNWnd);

	gset_ptr(PS_SELECT,NULL,-1L,-1L);
	SelectedMenu=msel_men(idMenu,aptPoint); /* メニューの動作 */

	return ExecuteMenu(SelectedMenu,aidWnd,aptPoint);
}


/* 命令キーを押しながら他のキーを押した時の処理 */
BOOL MenuKey(aKeyCode,aidWnd,aptPushed)
WORD aKeyCode;
WORD aidWnd;
POINT aptPushed;
{
	WORD SelectedMenu;

	/* メニューのマクロの処理 */
	SelectedMenu=mfnd_key(idMenu,aKeyCode);
	if(SelectedMenu>=0)
		return ExecuteMenu(SelectedMenu,aidWnd,aptPushed);
}

/* 全画面表示のインジケータの変更 */
void SwitchFullWindowIndicator()
{
	UWORD ItemNum;

	ItemNum=ID_MN_VIEW<<8 | ID_MN_VIEW_FULLWND;
	if(mchg_atr(idMenu,ItemNum,M_STAT) & 0x0001)
		mchg_atr(idMenu,ItemNum,M_NOSEL);
	else
		mchg_atr(idMenu,ItemNum,M_SEL);
}

BOOL GetFullWindowIndicator()
{
	UWORD ItemNum;

	ItemNum=ID_MN_VIEW<<8 | ID_MN_VIEW_FULLWND;
	if(mchg_atr(idMenu,ItemNum,M_STAT) & M_SEL)
		return TRUE;
	else
		return FALSE;
}
