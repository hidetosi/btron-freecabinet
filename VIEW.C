#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"

/* -------------------- �C�x���g�֌W -------------------- */


/* �I�����ꂽ�f�[�^�̍폜 */
void DeleteSelectedData(aidWnd,aisTemp)
WORD aidWnd;
BOOL aisTemp; /* TRUE=���ɓo�^����Ă��������폜���� */
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
		DeleteData(diDelete); /* �f�[�^�̍폜 */
		DrawOneRect(aidWnd,rctDeletedView);
		diDelete=diNext;
	}
	if(aisTemp)
		RecoverModification(iModified);
}


/* �}�E�X�̃{�^�����������Ƃ���ɂǂ̉��g�����邩�𒲂ׂ� */
DOCITERATOR FindVobj(aptClick,pwLocation)
POINT aptClick;
WORD* pwLocation;
{
	DOCITERATOR diCurrent;

	diCurrent=GetPrevious(NULL); /* �Ō�̃f�[�^���猟������ */
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
				return diCurrent; /* �������� */
		}
		diCurrent=GetPrevious(diCurrent);
	}
	return NULL; /* ������Ȃ����� */
}

/* �I�����ꂽ�f�[�^�̃R�s�[ */
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
		MoveRect(diNew,awDx,awDy); /* �f�[�^�̈ړ� */
		rctNewView=GetRect(diNew);
		omov_vob(idNewVobj,0,&rctNewView,V_DISPALL); /*�o�^�������g�̈ړ�*/
		UnSelectVobj(diCopy);
		SelectVobj(diNew);
		diCopy=GetNextSelectedVobj(diCopy);
	}
	SetBarValue(aidWnd);
}

/* �I�����ꂽ�f�[�^�̈ړ� */
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
		MoveRect(diMove,awDx,awDy); /* �f�[�^�̈ړ� */
		rctNewView=GetRect(diMove);
		omov_vob(GetVID(diMove),0,&rctNewView,V_NODISP); /*�o�^�������g�̈ړ�*/
		DrawOneRect(aidWnd,rctOldView);
		DrawOneRect(aidWnd,GetRect(diMove));
		diMove=GetNextSelectedVobj(diMove);
	}
	SetBarValue(aidWnd);
}

MakeNewObject(aidWnd,awDx,awDy,aidAtherWnd)
WORD aidWnd;
WORD awDx,awDy;
WORD aidAtherWnd; /* ���̃E�C���h�E�ɑ��鎞�Ɏg���B�����̃E�C���h�E�̎��� 0 */
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
		if(aidAtherWnd==0) /* �����̃E�C���h�E�ɐV�ł����Ƃ� */
			idNewVobj=onew_obj(GetVID(diMakeNew),(VLINKPTR)NULL);
		else{ /* ���̃E�C���h�E�ɐV�ł����Ƃ� */
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
		MoveRect(diNew,awDx,awDy); /* �f�[�^�̈ړ� */
		rctNewView=GetRect(diNew);
		if(aidAtherWnd==0) /* �o�^�������g�̈ړ� */
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


/* ���g��̈ʒu�ƃ|�C���^�̌`��̑Ή��B[0][n]���h���b�O���ĂȂ��Ƃ��B */
/* [1][n]���h���b�O���̎� */
WORD wPointStile[2][9]=
{{PS_MOVE,PS_MOVE,PS_SELECT,PS_MOVE,PS_RSIZ,PS_RSIZ,PS_RSIZ,PS_RSIZ,PS_MOVE},
 {PS_GRIP,PS_GRIP,PS_GRIP,  PS_GRIP,PS_PICK,PS_PICK,PS_PICK,PS_PICK,PS_GRIP}};

/* ���g�̃h���b�O�ړ� */
BOOL DragMove(aidWnd,aptStart,aisNewObject)
WORD aidWnd;
POINT aptStart;
BOOL aisNewObject; /* �V�ō쐬���ǂ��� */
{
	WORD idDrawEnv; /* �h���b�O�`��� */
	POINT ptMouse,ptOldMouse;
	WEVENT evDragEvent; /* �h���b�O���̃C�x���g */
	BOOL isCopy; /* copy �� move �� */
	BOOL isBreak; /* �h���b�O�����f���ꂽ���ǂ��� */
	ULONG lScrollTimer;

	/* �|�C���^�̌`��̕ύX */
	gset_ptr(PS_GRIP,(PTRIMAGE*)NULL,-1L,-1L);

	isBreak=FALSE;
	ptOldMouse=aptStart;
	lScrollTimer=0;
	idDrawEnv=wsta_drg(aidWnd,0); /* �h���b�O�̊J�n(lock �Ȃ�) */
	MakeSelectFrame(); /* �I��g���X�g���쐬 */
	DrawSelectFrame(-idDrawEnv,1,0,0); /* �g�̕\�� */

	while(wget_drg(&ptMouse,&evDragEvent)!=EV_BUTUP)
	{
		POINT ptAbsolute; /* �}�E�X�̃O���[�o�����W */
		WORD idToWnd; /* �}�E�X�̂���E�C���h�E ID */

		if(evDragEvent.s.type==EV_KEYDWN &&
			evDragEvent.e.data.key.code==TK_CAN) /* �~�L�[�������ꂽ�Ƃ� */
		{
			isBreak=TRUE;
			break; /* �h���b�O�̒��f */
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
		/* �}�E�X�̂���E�C���h�EID�𓾂� */
		ptAbsolute=ptMouse;
		gcnv_abs(idDrawEnv,&ptAbsolute); /* ��΍��W�ɕϊ� */
		wfnd_wnd(&ptAbsolute,(POINT*)NULL,&idToWnd);

		DrawSelectFrame(-idDrawEnv,0,ptOldMouse.c.h-aptStart.c.h,
						ptOldMouse.c.v-aptStart.c.v); /* �g�̏��� */

		if(idToWnd!=aidWnd) /* �}�E�X���E�C���h�E�̊O�ɏo���Ȃ� */
		{
			RECT rctAllDisplay;

			lScrollTimer=0;
			gloc_env(idDrawEnv,1); /* �h���b�O�`����̃��b�N */
			/* �S��ʂ�\�������`�ɂ��� */
			gget_fra(idDrawEnv,&rctAllDisplay);
			gset_vis(idDrawEnv,rctAllDisplay);
		}else{ /* �}�E�X�����̃E�C���h�E�̒��̎� */
			RECT rctWndFrame;
			ULONG lNow;

			gloc_env(idDrawEnv,0); /* �h���b�O�`����̃A�����b�N */
			/* �X�N���[�� */
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

			/* �E�C���h�E�̍�Ɨ̈��\�������`�ɂ��� */
			wget_wrk(aidWnd,&rctWndFrame);
			gset_vis(idDrawEnv,rctWndFrame);
		}

		DrawSelectFrame(-idDrawEnv,1,ptMouse.c.h-aptStart.c.h,
						ptMouse.c.v-aptStart.c.v); /* �g�̕\�� */
		ptOldMouse=ptMouse;
	}

	DrawSelectFrame(-idDrawEnv,0,ptOldMouse.c.h-aptStart.c.h,
					ptOldMouse.c.v-aptStart.c.v); /* �g�̏��� */
	wend_drg(); /* �h���b�O�̏I�� */
	FreeSelectFrame();

	if(isBreak)
		return; /* �h���b�O�����f���ꂽ�Ȃ牽�������ɏI�� */
	if(aptStart.l==ptMouse.l)
		return;

	if(evDragEvent.s.stat & (ES_BUT2|ES_CMD)) /* �{�^��2��������Ă��� */
		isCopy=TRUE;
	else
		isCopy=FALSE;
	if(evDragEvent.s.wid==aidWnd) /* ���̃E�C���h�E�Ȃ� */
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
	}else{ /* ���̃E�C���h�E�Ȃ� */
		WEVENT evPaste;
		POINT ptCursorMove; /* �������ʒu�ɓ\�荞�܂�邽�߂̏C���l */

		if(aisNewObject)
			MakeNewObject(aidWnd,0,0,evDragEvent.s.wid); /* ���o�^ */
		ptCursorMove=SendSelectedDataToTray(TRUE,aptStart);
		evPaste.r.type=EV_REQUEST;
		evPaste.r.cmd=W_PASTE;
		evPaste.r.wid=evDragEvent.s.wid;
		evPaste.s.pos=evDragEvent.s.pos;
		evPaste.s.pos.c.h+=ptCursorMove.c.h;
		evPaste.s.pos.c.v+=ptCursorMove.c.v;
		evPaste.s.time=evDragEvent.s.time;
		wsnd_evt(&evPaste); /* �\�荞�ݗv�������� */
		if(wwai_rsp((WEVENT*)NULL,W_PASTE,10000)!=W_ACK)
		{
			ErrPanel(ID_ST_NPASTE,FALSE);
			return;
		}
		wswi_wnd(evDragEvent.s.wid,(WEVENT*)NULL); /* �\�荞�ݐ�̃E�C���h�E��
														�t�H�[�J�X���ڂ� */
		if(aisNewObject)
			DeleteSelectedData(aidWnd,TRUE); /* ���̃E�C���h�E�ɓ\�荞�ނ��߂�
												���o�^���ꂽ�f�[�^������ */
		else if(!isCopy)
			DeleteSelectedData(aidWnd,FALSE);
	}
}

/* �ό`�p�̑I��g�̕\�� */
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


/* ��`��ό` */
RECT DragResizeRectFrame(aidWnd,aptStart,arctOriginal,awGripPoint)
WORD aidWnd;
POINT aptStart;
RECT arctOriginal; /* �ό`����������` */
WORD awGripPoint; /* �T�C�Y��ύX����ꏊ V_LTHD V_RTHD V_LBHD V_RBHD */
{
	WORD idDrawEnv; /* �h���b�O�`��� */
	SEL_RGN srResizeFrame;
	POINT ptMouse,ptOldMouse;
	WEVENT evDragEvent; /* �h���b�O���̃C�x���g */
	BOOL isBreak; /* �h���b�O�����f���ꂽ���ǂ��� */

	srResizeFrame.sts=0x0000;
	srResizeFrame.rgn.r=arctOriginal;
	isBreak=FALSE;
	ptOldMouse=aptStart;
	idDrawEnv=wsta_drg(aidWnd,0); /* �h���b�O�̊J�n(lock �Ȃ�) */
	DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
					aptStart,aptStart,1); /* �g�̕\�� */

	while(wget_drg(&ptMouse,&evDragEvent)!=EV_BUTUP)
	{
		POINT ptAbsolute; /* �}�E�X�̃O���[�o�����W */
		RECT rctWndFrame;

		if(evDragEvent.s.type==EV_KEYDWN &&
			evDragEvent.e.data.key.code==TK_CAN) /* �~�L�[�������ꂽ�Ƃ� */
		{
			isBreak=TRUE;
			break; /* �h���b�O�̒��f */
		}
		if(ptMouse.l==ptOldMouse.l)
		{
			ptOldMouse=ptMouse;
			continue;
		}
		DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
						aptStart,ptOldMouse,0); /* �g�̏��� */

		/* �X�N���[�� */
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
						aptStart,ptMouse,1); /* �g�̕\�� */
		ptOldMouse=ptMouse;
	}

	DrawResizeFrame(idDrawEnv,&srResizeFrame,arctOriginal,awGripPoint,
					aptStart,ptOldMouse,0); /* �g�̏��� */
	wend_drg(); /* �h���b�O�̏I�� */

	if(isBreak) /* �h���b�O�����f���ꂽ */
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


/* ���g�̃h���b�O�ό` */
void DragResize(aidWnd,aptStart,adiVobj,awGripPoint)
WORD aidWnd;
POINT aptStart;
DOCITERATOR adiVobj;
WORD awGripPoint; /* �T�C�Y��ύX����ꏊ V_LTHD V_RTHD V_LBHD V_RBHD */
{
	RECT rctOriginal;
	RECT rctResult;

	if(IsFixed(adiVobj))
		return;

	/* �|�C���^�̌`��̕ύX */
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


/* �h���b�O�g�őI�� */
void DragSelect(aidWnd,aptStart)
WORD aidWnd;
POINT aptStart;
{
	RECT rctOriginal;
	RECT rctResult;
	DOCITERATOR diCurrent;

	rctOriginal.p.lefttop=aptStart;
	rctOriginal.p.rightbot=aptStart;
	/* �g�̃h���b�O */
	rctResult=DragResizeRectFrame(aidWnd,aptStart,rctOriginal,V_RBHD);

	diCurrent=GetNext(NULL);
	while(diCurrent!=NULL)
	{
		RECT rctData;

		rctData=GetRect(diCurrent);
		if( rctData.c.left>rctResult.c.left &&
			rctData.c.top>rctResult.c.top &&
			rctData.c.right<rctResult.c.right &&
			rctData.c.bottom<rctResult.c.bottom ) /* �g�ɓ����Ă���Ȃ� */
		{
			SelectVobj(diCurrent);
		}
		diCurrent=GetNext(diCurrent);
	}
}


/* ��Ɨ̈�Ń}�E�X�̃{�^���������ꂽ�Ƃ��̏��� */
void PushWorkArea(apevWnd)
WEVENT* apevWnd;
{
	WORD idVobj;
	WORD wLocation;
	DOCITERATOR diPointed;

	/* ���g�̈ʒu�̌��� */
	diPointed=FindVobj(apevWnd->s.pos,&wLocation);
	if(diPointed==NULL) /* ���g���w���Ă��Ȃ��ꍇ */
	{
		DrawSelectFrame(apevWnd->s.wid,0,0,0);
		FreeSelectFrame();
		if(!(apevWnd->s.stat&ES_LSHFT)||(apevWnd->s.stat&ES_RSHFT))
			UnSelectAllVobj();
		if(wchk_dck(apevWnd->s.time)==W_PRESS)
			DragSelect(apevWnd->s.wid,apevWnd->s.pos); /* �h���b�O�g�őI�� */
	}else{
		WORD wClick;

		/* �|�C���^�̌`��̕ύX */
		gset_ptr(wPointStile[0][wLocation],(PTRIMAGE*)NULL,-1L,-1L);

		DrawSelectFrame(apevWnd->s.wid,0,0,0); /* �I��g������ */
		FreeSelectFrame(); /* �I��g���X�g�̃���������� */

		if(IsBackground(diPointed)) /* �w�i������Ă���ꍇ */
		{ /* ���ʈ������� */
			if( (wLocation==V_PICT)&&(wchk_dck(apevWnd->s.time)==W_DCLICK) )
				oexe_apg(GetVID(diPointed),0); /* �f�t�H���g�A�v���̎��s */
			else
				UnSelectAllVobj();
			MakeSelectFrame(); /* �I��g���X�g���쐬 */
			DrawSelectFrame(apevWnd->s.wid,1,0,0); /* �I��g��\�� */
			return;
		}

		if((apevWnd->s.stat&ES_LSHFT)||(apevWnd->s.stat&ES_RSHFT))
		{ /* �V�t�g��������Ă����Ƃ� */
			SwitchVobjSelect(diPointed); /* �I���󋵂𔽓]���� */
			MakeSelectFrame(); /* �I��g���X�g���쐬 */
			DrawSelectFrame(apevWnd->s.wid,1,0,0); /* �I��g��\�� */
			return;
		}

		if(IsSelected(diPointed)==FALSE)
		{ /* �I������Ă��Ȃ����������������Ƃ� */
			UnSelectAllVobj();
			SwitchVobjSelect(diPointed); /* �I�����ꂽ���Ƃɂ��� */
		}

		wClick=wchk_dck(apevWnd->s.time);
		switch(wLocation)
		{
		case V_NAME:
			if(wClick==W_DCLICK)
			{ /* ���g���̕ύX */
				if(ochg_nam(GetVID(diPointed),(TPTR)NULL)==1)
					DrawOneRect(apevWnd->s.wid,GetRect(diPointed));
				break;
			}
		case V_RELN:
			if(wClick==W_DCLICK)
			{ /* �����̕ύX */
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
				oexe_apg(GetVID(diPointed),0); /* �f�t�H���g�A�v���̎��s */
				break;
			}
		case V_WORK:
		case V_FRAM:
			switch(wClick)
			{
			case W_PRESS:
				/* �h���b�O�ړ� */
				DragMove(apevWnd->s.wid,apevWnd->s.pos,FALSE);
				break;
			case W_QPRESS:
				/* �V�ō쐬 */
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
				/* �h���b�O�ό` */
				DragResize(apevWnd->s.wid,apevWnd->s.pos,diPointed,wLocation);
			}
			break;
		}
	}
	MakeSelectFrame(); /* �I��g���X�g���쐬 */
	DrawSelectFrame(apevWnd->s.wid,1,0,0); /* �I��g��\�� */
}

void ExecSelectedVobj()
{
	DOCITERATOR diCurrent;

	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		oexe_apg(GetVID(diCurrent),0); /* �f�t�H���g�A�v���̎��s */
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

/* �L�[���������Ƃ��̏��� */
BOOL KeyDown(aevWnd,aidWnd)
WEVENT aevWnd;
WORD aidWnd;
{
	static KEYTAB* pktKey=NULL;
	TCODE tcAlphaKey;

	if(pktKey==NULL)
	{ /*�L�[�{�[�h�\�̎��o�� */
		WORD wSize;

		wSize=get_ktb((KEYTAB*)NULL);
		get_lmb(&pktKey,(LONG)wSize,NOCLR);
		get_ktb(pktKey);
	}
	tcAlphaKey=pktKey->kct[pktKey->key_max*pktKey->kct_sel[0x01]+
							aevWnd.e.data.key.keytop];
	if(aevWnd.s.stat & ES_CMD) /* ���߃L�[��������Ă��� */
	{
		switch(aevWnd.e.data.key.code)
		{
		/* �J�[�\���L�[ */
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
	}else if(aevWnd.s.stat & (ES_LSHFT | ES_RSHFT)) /* �V�t�g��������Ă��� */
	{
		switch(aevWnd.e.data.key.code)
		{
		case TK_BS: /* �o�b�N�X�y�[�X */
			return FALSE; /* �I�� */
		case 0x000D: /* ���^�[���L�[ */
			ExecSelectedVobj(); /* ���s */
			return FALSE; /* �I�� */
		}
	}else{
		switch(tcAlphaKey/*aevWnd.e.data.key.code*/)
		{
		case TK_DEL: /* �f���[�g�L�[ */
			DrawSelectFrame(aidWnd,0,0,0); /* �I��g������ */
			FreeSelectFrame(); /* �I��g���X�g�̃���������� */
			DeleteSelectedData(aidWnd,FALSE); /* �폜 */
			break;
		case TK_NL: /* ���^�[���L�[ */
			ExecSelectedVobj(); /* ���s */
			break;
		case TK_BS: /* �o�b�N�X�y�[�X */
			ChangeForcusToParentWindow(aidWnd);
							/* �e�E�C���h�E�Ƀt�H�[�J�X���ڂ� */
			break;
		case TK_i:
			SelectedVobjInfo(); /* ���g�Ǘ����̕\�� */
			break;
		}
	}

	return TRUE;
}


/* �S�ʕ\���؂�ւ� */
void SwitchFullWindow(aidWnd)
WORD aidWnd;
{
	/* �C���W�P�[�^�̕ύX */
	SwitchFullWindowIndicator();

	/* �S�ʕ\���؂�ւ� */
	if(wchg_wnd(aidWnd,NULL,W_MOVE)>0)
		DrawView(aidWnd);
}

void NullEvent(aevWnd,aidWnd)
WEVENT aevWnd;
WORD aidWnd;
{
	WORD wNewPointStile;

	DrawSelectFrame(aidWnd,-1,0,0);
	if(aevWnd.s.wid!=aidWnd) /* ���̃E�C���h�E��Ȃ牽�����Ȃ� */
		return;
	if(aevWnd.s.stat&ES_CMD) /* ���߃L�[��������Ă���Ƃ� */
		wNewPointStile=PS_MENU;
	else if(aevWnd.s.cmd!=W_WORK) /* ��Ɨ̈�ȊO�̎� */
		return;
	else if((aevWnd.s.stat&ES_LSHFT)||(aevWnd.s.stat&ES_RSHFT))
		/* �V�t�g��������Ă���Ƃ� */
		wNewPointStile=PS_MODIFY;
	else /* ��Ɨ̈�̎� */
	{
		DOCITERATOR diPointed;
		WORD wLocation;

		/* ���g�̈ʒu�̌��� */
		diPointed=FindVobj(aevWnd.s.pos,&wLocation);
		if(diPointed!=NULL)
		{ /* �|�C���^�̌`��̕ύX */
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

	TakeFrontOrBackSelectedData(FALSE);/*�w�i�����ꂽ���ԍŏ��Ɏ����Ă��� */
	diCurrent=GetNextSelectedVobj(NULL);
	while(diCurrent!=NULL)
	{
		BackgroundVobj(diCurrent); /* �w�i�� */
		DrawOneRect(aidWnd,GetRect(diCurrent));
		diCurrent=GetNextSelectedVobj(diCurrent);
	}
}

/* �v���O�������I������O�Ƀt�@�C���̃Z�[�u�̏��������� */
BOOL EndProg()
{
	if(IsDataModified())
	{
		gset_ptr(PS_SELECT,(PTRIMAGE*)NULL,-1L,-1L);
		switch(pact_err(ID_PN_SAVE,(TPTR)NULL,(TPTR)NULL,(TPTR)NULL))
		{
		case ID_PN_SAVE_OK:
			if(!SaveFile())
				return TRUE; /* �ۑ��Ɏ��s�����Ȃ�ۑ����Ȃ� */
			return FALSE; /* �I�� */
		case ID_PN_SAVE_NO:
			return FALSE; /* �I�� */
		case ID_PN_SAVE_CANCEL:
			return TRUE;
		}
	}else if(IsSegmentOnlyModified())
		SaveFile(); /* ��������ۑ� */
	return FALSE;
}


/* �G���[�p�l���̕\�� */
void ErrPanel(awMessage,aisDouble)
WORD awMessage; /* �\������X�g�����O�������f�[�^�{�b�N�X�̔ԍ� */
				/* ������������Ȃ��Ȃ� 0 ���w�肷�� */
BOOL aisDouble; /* �����s=TRUE 1�s=FALSE */
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