#include <STD.h>
#include <mmi.h>
#include "fcab.h"
#include "view.h"
#include "doc.h"
#include "databox.h"


#define REDISPRECTNUM 10 /* �ĕ\���p�̋�`���X�g�̐� */


/* -------------------- �I��g�֌W -------------------- */


SEL_LIST* slSelectFrameList=NULL; /* �I��g���X�g */

/* �I��g�̕\�� */
void DrawSelectFrame(aidWnd,awMode,awDx,awDy)
WORD aidWnd; /* �l���}�C�i�X�Ȃ�`��� */
WORD awMode;
WORD awDx;
WORD awDy;
{
	WORD idDrawEnv;
	RECT rctWork;

	if(slSelectFrameList==NULL)
		return;
	if(aidWnd>0) /* �E�C���h�E�ɕ`���Ƃ� */
	{
		idDrawEnv=wget_gid(aidWnd);
		wget_wrk(aidWnd,&rctWork);
		gset_vis(idDrawEnv,rctWork); /* �̈�̃Z�b�g */
	}else /* �`����ɕ`���Ƃ� */
		idDrawEnv=-aidWnd;
	adsp_slt(idDrawEnv,slSelectFrameList,awMode,awDx,awDy); /* �I��g��\�� */
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


/* -------------------- �`��֌W -------------------- */


void DrawOneVobj(aidVobj,aidWnd)
WORD aidVobj;
WORD aidWnd;
{
	WORD idDrawEnv;
	RECT rctWork;

	idDrawEnv=wget_gid(aidWnd); /* �`����̎擾 */
	wget_wrk(aidWnd,&rctWork); /* ��Ɨ̈�̎擾 */
	gset_vis(idDrawEnv,rctWork); /* �̈�̃Z�b�g */
	odsp_vob(aidVobj,NULL,V_DISPALL); /* ���g�̕\�� */
}

void DrawOneRect(aidWnd,arctDraw)
WORD aidWnd;
RECT arctDraw;
{
	WORD idDrawEnv;
	BYTE* pVIDList,*p;
	DOCITERATOR diCurrent;

/* b_printf("view(%d,%d)-(%d,%d)\n",arctDraw.c.left,arctDraw.c.top,arctDraw.c.right,arctDraw.c.bottom); */
	idDrawEnv=wget_gid(aidWnd); /* �`����̎擾 */
	gset_vis(idDrawEnv,arctDraw); /* �̈�̃Z�b�g */

	wera_wnd(aidWnd,&arctDraw); /* �w�i�F�œh��Ԃ� */
	/* ID ���X�g�̍쐬 */
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
		{ /* �\���̈�ɓ����Ă����� */
			*((WORD*)p)=GetVID(diCurrent);
			p+=sizeof(WORD);
			*((RECT*)p)=arctDraw;
			p+=sizeof(RECT);
/* b_printf("ID %d (%d,%d)-(%d,%d)\n",GetVID(diCurrent),rctVobj.c.left,rctVobj.c.top,rctVobj.c.right,rctVobj.c.bottom); */
		}
	}
	*((WORD*)p)=0;

	/* ���g�̕\�� */
	odsp_vob(0x8000,pVIDList,V_DISPALL);

	rel_lmb(pVIDList);
}


/* �ĕ\������������ */
void DrawView(aidWnd)
WORD aidWnd;
{
	static RECTLIST rlListItem[REDISPRECTNUM];
	static RLPTR prlList=NULL; /* �ĕ\���p�̋�`���X�g */

	DrawSelectFrame(aidWnd,0,0,0);

	if(prlList==NULL)
	{ /* �ĕ\���p�̋�`���X�g�̏����� */
		int i;

		prlList=rlListItem;
		for(i=0; i<REDISPRECTNUM-1; i++)
			rlListItem[i].r_next=&rlListItem[i+1];
		rlListItem[REDISPRECTNUM].r_next=NULL;
	}

	do {
		int iRectNum,i;
		RECT rctDraw;

		iRectNum=wsta_dsp(aidWnd,&rctDraw,prlList); /* �ĕ\���J�n */
		if(iRectNum==0)
			break;
		if(iRectNum>REDISPRECTNUM) /* �ĕ\���̕K�v�ȋ�`�̐��������Ƃ� */
			DrawOneRect(aidWnd,rctDraw);
		else /* �ĕ\���̕K�v�ȋ�`�̐������Ȃ��Ƃ� */
			for(i=0; i<iRectNum; i++)
				DrawOneRect(aidWnd,rlListItem[i].rcomp); /* 1��1�`�� */
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