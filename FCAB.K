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
WORD* apwUniqueData; /* �ŗL�f�[�^ */
BOOL aisDisp; /* �J�������g���ǂ��� */
{
	WORD idWnd; /* �E�B���h�EID */
	RECT rctWnd,rctWork; /*�E�B���h�E�̕\���ʒu*/
	TCODE tFilename[21]; /* ���g�� */
	PATTERN ptnBack; /* �w�i�p�^�[�� */

	ptnBack.spat.kind=0;
	ptnBack.spat.hsize=8;
	ptnBack.spat.vsize=1;
	ptnBack.spat.bgcol=-1;
	ptnBack.spat.mask=FILL100;

	/* �ŗL�f�[�^�̓ǂݍ��� */
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

	if(aisDisp) /* �J�������g�Ȃ� */
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
	}else{ /* ���g�̃I�[�v���Ȃ� */
		ptnBack.spat.fgcol=apMsg->bgcol;

		fil_sts(&(apMsg->lnk),tFilename,NULL,NULL); /* ���g���̊l�� */

		/*�E�B���h�E���I�[�v��*/
		idWnd = wopn_wnd(WA_SIZE|WA_HHDL|WA_VHDL|WA_RBAR|WA_BBAR,
					apMsg->pwid,&rctWnd,NULL,6,tFilename,&ptnBack,NULL);
		if (idWnd < 0)
			ErrEnd(L"�E�C���h�E���J���܂���ł���");
		wset_wrk(idWnd,&rctWork); /* �f�[�^���̕\������ʒu��ݒ� */
		return idWnd;
	}

}

void MAIN(apMsg)
M_EXECREQ* apMsg;
{
	WORD idWnd; /* �E�B���h�EID */
	WORD idDatabox; /* �f�[�^�{�b�N�X�̃t�@�C��ID */
	WORD* pwUniqueData; /* �ŗL�f�[�^ */
	WDSTAT wdsWindowState; /* �E�C���h�E�̏�� */

	gset_ptr(PS_BUSY,NULL,-1L,-1L); /* �|�C���^����݂̂ɂ��� */

	chg_wrk(NULL);

	idDatabox=dopn_dat(&(apMsg->self)); /* �f�[�^�{�b�N�X�̃I�[�v�� */
	if( idDatabox < 0)
		ErrEnd(L"�f�[�^�{�b�N�X���J���܂���ł���");

	/* �N���̎d���ɂ�铮��̈Ⴂ */
	switch(apMsg->type)
	{
	case EXECREQ:
		if(!(apMsg->mode & 0x0002)) /* ���g�N���̎� */
		{
			if(apMsg->mode & 0x0001) /* �f�t�H���g�N���łȂ��� */
				pwUniqueData=NULL;
			else /* �f�t�H���g�N���̎� */
				pwUniqueData=apMsg->info;

			if(!FileOpen(apMsg->vid,apMsg->lnk))
				ErrEnd("\0"); /* �t�@�C�����J���Ȃ����� */
			idWnd=OpenWindow(apMsg,pwUniqueData,(BOOL)FALSE);
			osta_prc(apMsg->vid,idWnd); /* ���g�����̊J�n */
			ReadFile(idWnd); /* ���g�̓ǂݍ��� */

			break; /* ���ɐi�� */
		}
	case DISPREQ:
		pwUniqueData=apMsg->info;

		if(!FileOpen(apMsg->vid,apMsg->lnk))
			ErrEnd("\0"); /* �t�@�C�����J���Ȃ����� */
		idWnd=OpenWindow(apMsg,pwUniqueData,(BOOL)TRUE);

		ReadFile(idWnd); /* ���g�̓ǂݍ��� */

		oend_req(apMsg->vid,0);
		ext_prc(0); /* �I�� */

	case PASTEREQ:
		oend_req(apMsg->vid,1);
	default:
		ErrEnd(L"���s�̎d�����s���ł�");
	}

	MenuInit(); /* ���j���[�̏����� */

	InitScrollBar(idWnd); /* �X�N���[���o�[�̏����� */

	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* �|�C���^�����ɖ߂� */

	for (;;)  /*�C�x���g���[�v*/
	{
		WEVENT evWnd; /* �C�x���g */

		wget_evt(&evWnd, WAIT);	/*�C�x���g���擾*/
		if(!EventExec(evWnd,idWnd))
		{
			if(!EndProg())
				break; /* �I�� */
		}
	}
	dcls_dat(idDatabox); /* �f�[�^�{�b�N�X�̃N���[�Y */

	/* �v���O�����̏I������ */
	gset_ptr(PS_BUSY, NULL, -1L, -1L); /* �|�C���^��ύX */
	wget_sts(idWnd,&wdsWindowState,NULL); /* �E�C���h�E�̏�Ԃ̎擾 */
	WriteUniqueData(wdsWindowState.r,wdsWindowState.wr);
	wcls_wnd(idWnd, CLR); /* �E�C���h�E�̃N���[�Y */
	gset_ptr(PS_SELECT,NULL,-1L,-1L); /* �|�C���^�����ɖ߂� */
	ext_prc(0);
}