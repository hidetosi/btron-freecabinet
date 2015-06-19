#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"


/* �}�E�X�̃{�^�����������Ƃ��̏��� */
BOOL MouseButtonDown(aidWnd,aevWnd)
WORD aidWnd;
WEVENT *aevWnd;
{
	if(aidWnd!=aevWnd->s.wid)
		return TRUE;
	switch(aevWnd->s.cmd)
	{
	case W_LTHD: /* �n���h�� */
	case W_RTHD:
	case W_LBHD:
	case W_RBHD:
		switch(wchk_dck(aevWnd->s.time))
		{
		case W_DCLICK:
			SwitchFullWindow(aidWnd); /* �S�ʕ\���؂�ւ� */
			SetBarValue(aidWnd);
			break;

		case W_PRESS:
		case W_QPRESS:
			/* �h���b�O�ό`���� */
			if(wrsz_drg(aevWnd,NULL,NULL)==1)
				DrawView(aevWnd->s.wid);
			if(GetFullWindowIndicator()==TRUE)
				SwitchFullWindowIndicator();
			SetBarValue(aidWnd);
			break;
		}
		break;

	case W_PICT: /* �s�N�g�O���� */
		switch (wchk_dck(aevWnd->s.time))
		{
		case W_DCLICK: /* �_�u���N���b�N�̎� */
			return FALSE; /* �I�� */
		case W_PRESS:
			break; /* �ړ� */
		default:
			return TRUE;
		}

	case W_FRAM: /* �g */
	case W_TITL: /* �^�C�g���o�[ */
		/* �h���b�O�ړ����� */
		if(wmov_drg(aevWnd, NULL)==1)
			DrawView(aevWnd->s.wid);
		if(GetFullWindowIndicator()==TRUE)
			SwitchFullWindowIndicator();
		break;

	case W_RBAR: /* �E�X�N���[���o�[ */
		ActScrollBar(RIGHTBAR,aevWnd);
		break;
		
	case W_BBAR: /* ���X�N���[���o�[ */
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
		case W_REDISP:/* �ĕ\���v�� */
			DrawView(aidWnd);
			break;
		case W_PASTE: /* �\���ݗv��*/
			DrawSelectFrame(aidWnd,0,0,0); /* �I��g�̏��� */
			FreeSelectFrame();
			PasteFromTray(aidWnd,TRUE,aevWnd.s.pos);
			wrsp_evt(&aevWnd,0);
			MakeSelectFrame();
			SetBarValue(aidWnd);
			break;
		case W_DELETE: /* �E�C���h�E�̃N���[�Y */
		case W_FINISH: /* �����I�� */
			ForceNoSave();
			wrsp_evt(&aevWnd,0);
			return FALSE;
		case W_VOBJREQ: /* ���g�v���C�x���g */
			switch(aevWnd.g.data[2])
			{
			case 0: /* ���̂̕ύX */
			case 1: /* ��������ԂɂȂ��� */
			case 2: /* ��������Ԃ��������ꂽ */
			case 3: /* ���g��ԂɂȂ��� */
			case 4: /* ���g��Ԃ��������ꂽ */
/* b_printf("request=%d ID=%d\n",aevWnd.g.data[2],aevWnd.g.data[3]); */
				{ /* �Ȃ����g�Ɋo���̂Ȃ����gID(ID=1)�̗v��������̂�
				 		���̑΍�����Ă���܂��B���������� 1B �̃o�O�H */
					DOCITERATOR diChange;
					if((diChange=GetIterator(aevWnd.g.data[3]))!=NULL)
						DrawOneRect(aidWnd,GetRect(diChange));
												/* ���g�̍ĕ`�� */
				}
				break;
			case 5: /* ���g�E�C���h�E�ɂȂ��� */
			case 6: /* ���g�E�C���h�E�������ɂȂ��� */
				wreq_dsp(aidWnd); /* �ĕ`�� */
				break;
			case 15: /* ���g�̕ύX�ʒm */
				VobjSegmentModified(aevWnd.g.data[3]); /* �ύX�̃`�F�b�N */
				break;
			case 16: /* ���g�̑}���v�� */
			case 17: /* ���g�̑}���v�� */
				DrawSelectFrame(aidWnd,0,0,0);
				AddVobjByID(aevWnd.g.data[3]); /* ���g�̒ǉ� */
				DrawOneVobj(aevWnd.g.data[3],aidWnd); /* �ǉ��������g�̕`�� */
				SetBarValue(aidWnd);
				break;
			case 128: /* �ꎞ�t�@�C���ւ̊i�[�v�� */
				break;
			}
			break;
		}
		break;
	case EV_RSWITCH:
		DrawView(aidWnd);
	case EV_SWITCH:
	case EV_BUTDWN: /* �}�E�X�̃{�^���������ꂽ�Ƃ� */
		return MouseButtonDown(aidWnd,&aevWnd);
	case EV_MENU: 
		return OpenMenu(aevWnd.s.pos,aidWnd);
	case EV_AUTKEY:
	case EV_KEYDWN: /* �L�[�������ꂽ�Ƃ� */
		return KeyDown(aevWnd,aidWnd);
	case EV_INACT:
		DrawSelectFrame(aidWnd,0,0,0); /* �I��g�̏��� */
		break;
	case EV_DEVICE:
		oprc_dev(&aevWnd,NULL,0);
		break;
	case EV_MSG:
		{ /* ���b�Z�[�W�͎̂Ă� */
			WORD aMsg[32];
			rcv_msg(0x7fffffffL,aMsg,sizeof(aMsg),NOWAIT|CLR);
			break;
		}
	}
	return TRUE;
}
