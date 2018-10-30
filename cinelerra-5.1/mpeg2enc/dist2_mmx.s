;
;  dist2_mmx.s:  mmX optimized squared distance sum
;
;  Original believed to be Copyright (C) 2000 Brent Byeler
;
;  This program is free software; you can reaxstribute it and/or
;  modify it under the terms of the GNU General Public License
;  as published by the Free Software Foundation; either version 2
;  of the License, or (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
;

; total squared difference between two (16*h) blocks
; including optional half pel interpolation of [ebp+8] ; blk1 (hx,hy)
; blk1,blk2: addresses of top left pels of both blocks
; lx:        distance (in bytes) of vertically adjacent pels
; hx,hy:     flags for horizontal and/or vertical interpolation
; h:         height of block (usually 8 or 16)
; mmX version

global dist2_mmx
; int dist2_mmx(unsigned char *blk1, unsigned char *blk2,
;                 int lx, int hx, int hy, int h)

; mm7 = 0

; eax = pblk1 
; ebx = pblk2
; ecx = temp
; edx = distance_sum
; edi = h
; esi = lx

		;; 
		;;  private constants needed
		;; 

SECTION .data
align 16
twos:	
			dw	2
			dw	2
			dw	2
			dw	2

align 32
dist2_mmx:
	push ebp			; save frame pointer
	mov ebp, esp		; link
	push ebx
	push ecx
	push edx
	push esi     
	push edi

	mov		esi, [ebp+16] ; lx
	mov     eax, [ebp+20] ; hx
	mov     edx, [ebp+24] ; hy
	mov     edi, [ebp+28] ; h

    pxor      mm5, mm5      ; sum
	test      edi, edi     ; h = 0?
	jle       near d2exit

	pxor	  mm7, mm7     ; get zeros i mm7

	test      eax, eax     ; hx != 0?
	jne       near d2is10
	test      edx, edx     ; hy != 0?
	jne       near d2is10

	mov       eax, [ebp+8]
    mov       ebx, [ebp+12]
	jmp 	 d2top00

align 32
d2top00:
        movq      mm0, [eax]
        movq      mm1, mm0
        punpcklbw mm0, mm7
        punpckhbw mm1, mm7

        movq      mm2, [ebx]
        movq      mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

        psubw     mm0, mm2
        psubw     mm1, mm3
        pmaddwd   mm0, mm0
        pmaddwd   mm1, mm1
        paddd     mm0, mm1

        movq      mm1, [eax+8]
        movq      mm2, mm1
        punpcklbw mm1, mm7
        punpckhbw mm2, mm7

        movq      mm3, [ebx+8]
        movq      mm4, mm3
        punpcklbw mm3, mm7
        punpckhbw mm4, mm7

        psubw     mm1, mm3
        psubw     mm2, mm4
        pmaddwd   mm1, mm1
        pmaddwd   mm2, mm2
        paddd     mm1, mm2

        paddd     mm0, mm1
	
		;; Accumulate sum in edx... we use mm5
		;movd	  ecx, mm0
        ;add       edx, ecx
	    ;psrlq	  mm0, 32
	    ;movd	  ecx, mm0
        ;add       edx, ecx
		paddd    mm5, mm0

	add       eax, esi
	add       ebx, esi
	dec       edi
	jg        d2top00
	jmp       d2exit


d2is10:
	test      eax, eax
	je        near d2is01
	test      edx, edx
	jne       near d2is01

 
	mov       eax, [ebp+8] ; blk1
	mov       ebx, [ebp+12] ; blk1

	pxor	  mm6, mm6    ; mm6 = 0 and isn't changed anyplace in the loop..
	pcmpeqw	  mm1, mm1
	psubw	  mm6, mm1
	jmp		  d2top10
	
align 32
d2top10:
	movq	  mm0, [eax]
	movq	  mm1, mm0
	punpcklbw mm0, mm7
	punpckhbw mm1, mm7
	movq	  mm2, [eax+1]
	movq	  mm3, mm2
	punpcklbw mm2, mm7
	punpckhbw mm3, mm7
	paddw	  mm0, mm2
	paddw	  mm1, mm3
	paddw	  mm0, mm6   ; here we add mm6 = 0.... weird...
	paddw	  mm1, mm6
	psrlw	  mm0, 1
	psrlw	  mm1, 1

	movq	  mm2, [ebx]
	movq	  mm3, mm2
	punpcklbw mm2, mm7
	punpckhbw mm3, mm7

        psubw     mm0, mm2
        psubw     mm1, mm3
        pmaddwd   mm0, mm0
        pmaddwd   mm1, mm1
        paddd     mm0, mm1

	movq	  mm1, [eax+8]
	movq	  mm2, mm1
	punpcklbw mm1, mm7
	punpckhbw mm2, mm7
	movq	  mm3, [eax+9]
	movq	  mm4, mm3
	punpcklbw mm3, mm7
	punpckhbw mm4, mm7
	paddw	  mm1, mm3
	paddw	  mm2, mm4
	paddw	  mm1, mm6
	paddw	  mm2, mm6
	psrlw	  mm1, 1
	psrlw	  mm2, 1

	movq	  mm3, [ebx+8]
	movq	  mm4, mm3
        punpcklbw mm3, mm7
        punpckhbw mm4, mm7

        psubw     mm1, mm3
        psubw     mm2, mm4
        pmaddwd   mm1, mm1
        pmaddwd   mm2, mm2
        paddd     mm1, mm2


	paddd	  mm0, mm1
		; Accumulate mm0 sum on edx... we'll use mm5 for this and add up at the end
		; movd	  ecx, mm0
        ; add       edx, ecx
		; psrlq	  mm0, 32
		; movd	  ecx, mm0
        ; add       edx, ecx
	paddd     mm5, mm0
	add       eax, esi
	add       ebx, esi
	dec       edi
	jg        near d2top10
	
	
	jmp       d2exit

d2is01:
	test      eax, eax
	jne       near d2is11
	test      edx, edx
	je        near d2is11

	mov       eax, [ebp+8] ; blk1
	mov       edx, [ebp+12] ; blk2
	mov       ebx, eax
	add       ebx, esi ;  blk1 + lx

	pxor	  mm6, mm6
	pcmpeqw	  mm1, mm1
	psubw	  mm6, mm1  ; mm6 = 1
	jmp		  d2top01
	
align 32
d2top01:
	movq	  mm0, [eax]
	movq	  mm1, mm0
	punpcklbw mm0, mm7
	punpckhbw mm1, mm7
	movq	  mm2, [ebx]
	movq	  mm3, mm2
	punpcklbw mm2, mm7
	punpckhbw mm3, mm7
	paddw	  mm0, mm2
	paddw	  mm1, mm3
	paddw	  mm0, mm6
	paddw	  mm1, mm6
	psrlw	  mm0, 1
	psrlw	  mm1, 1

	movq	  mm2, [edx]
	movq	  mm3, mm2
    punpcklbw mm2, mm7
    punpckhbw mm3, mm7

    psubw     mm0, mm2
    psubw     mm1, mm3

	pmaddwd   mm0, mm0
    pmaddwd   mm1, mm1
    paddd     mm0, mm1

	movq	  mm1, [eax+8]
	movq	  mm2, mm1
	punpcklbw mm1, mm7
	punpckhbw mm2, mm7
	
	movq	  mm3, [ebx+8]
	movq	  mm4, mm3
	punpcklbw mm3, mm7
	punpckhbw mm4, mm7

	paddw	  mm1, mm3
	paddw	  mm2, mm4
	paddw	  mm1, mm6
	paddw	  mm2, mm6
	psrlw	  mm1, 1
	psrlw	  mm2, 1

	movq	  mm3, [edx+8]
	movq	  mm4, mm3
    punpcklbw mm3, mm7
    punpckhbw mm4, mm7

    psubw     mm1, mm3
    psubw     mm2, mm4

	pmaddwd   mm1, mm1
    pmaddwd   mm2, mm2
    paddd     mm0, mm1
	paddd	  mm0, mm2

	;; Accumulate in "s" - we use mm5 for the purpose
	;;
	;movd	  ecx, mm0
    ;add       s, ecx
	;psrlq	  mm0, 32
	;movd	  ecx, mm0
	;add	  	  s, ecx
	paddd     mm5, mm0

	;; Originally this moved 
	mov       eax, ebx    ; eax = eax + lx
	add       edx, esi    ; edx = edx + lx
	add       ebx, esi    ; ebx = ebx + lx
	dec       edi
	jg        near d2top01
	jmp       d2exit

d2is11:
	mov       eax, [ebp+8] ; blk1
	mov       edx, [ebp+12] ; blk2
	mov       ebx, eax  ;  blk1
	add       ebx, esi  ; ebx = blk1 + lx
	jmp		  d2top11
	
align 32
d2top11:
	movq	  mm0, [eax]
	movq	  mm1, mm0
	punpcklbw mm0, mm7
	punpckhbw mm1, mm7
	movq	  mm2, [eax+1]
	movq	  mm3, mm2
	punpcklbw mm2, mm7
	punpckhbw mm3, mm7
	paddw	  mm0, mm2
	paddw	  mm1, mm3
	movq	  mm2, [ebx]
	movq	  mm3, mm2
	punpcklbw mm2, mm7
	punpckhbw mm3, mm7
	movq	  mm4, [ebx+1]
	movq	  mm6, mm4
	punpcklbw mm4, mm7
	punpckhbw mm6, mm7
	paddw	  mm2, mm4
	paddw	  mm3, mm6
	paddw	  mm0, mm2
	paddw	  mm1, mm3
	;pxor	      mm6, mm6    ; mm6 = 0
	;pcmpeqw	  mm5, mm5    ; mm5 = -1
	;psubw	      mm6, mm5    ; mm6 = 1
	;paddw	      mm6, mm6    ; mm6 = 2
	movq      mm6, [twos]
	paddw	  mm0, mm6    ; round mm0
	paddw	  mm1, mm6    ; round mm1
	psrlw	  mm0, 2
	psrlw	  mm1, 2

	movq	  mm2, [edx]
	movq	  mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

        psubw     mm0, mm2
        psubw     mm1, mm3
        pmaddwd   mm0, mm0
        pmaddwd   mm1, mm1
        paddd     mm0, mm1

	movq	  mm1, [eax+8]
	movq	  mm2, mm1
	punpcklbw mm1, mm7
	punpckhbw mm2, mm7
	
	movq	  mm3, [eax+9]
	movq	  mm4, mm3
	punpcklbw mm3, mm7
	punpckhbw mm4, mm7
	
	paddw	  mm1, mm3
	paddw	  mm2, mm4
	
	movq	  mm3, [ebx+8]
	movq	  mm4, mm3
	punpcklbw mm3, mm7
	punpckhbw mm4, mm7
	paddw     mm1, mm3
	paddw     mm2, mm4 
	
	movq	  mm3, [ebx+9]
	movq	  mm4, mm3
	punpcklbw mm3, mm7
	punpckhbw mm4, mm7

	paddw	  mm1, mm3
	paddw	  mm2, mm4

	;pxor	  mm6, mm6    ; Zero mm6
	;pcmpeqw	  mm5, mm5    ; mm5 = -1
	;psubw	  mm6, mm5    ; mm6 = 1
	;paddw	  mm6, mm6    ; mm6 = 2
	;paddw	  mm1, mm6    ; round mm1 and mm2
	;paddw    mm2, mm6
	movq      mm6, [twos]
	paddw     mm1, mm6
	paddw	  mm2, mm6
	
	psrlw	  mm1, 2
	psrlw	  mm2, 2

	movq	  mm3, [edx+8]
	movq	  mm4, mm3
        punpcklbw mm3, mm7
        punpckhbw mm4, mm7

        psubw     mm1, mm3
        psubw     mm2, mm4
        pmaddwd   mm1, mm1
        pmaddwd   mm2, mm2
        paddd     mm1, mm2

	paddd	  mm0, mm1
	
	;;
	;; Accumulate the result in "s" we use mm6 for the purpose...
	;movd	  ecx, mm0
    ;    add       s, ecx
	;psrlq	  mm0, 32
	;movd	  ecx, mm0
	;add	  s, ecx
	paddd     mm5, mm0

	mov       eax, ebx    ; ahem ebx = eax at start of loop and wasn't changed...
	add       ebx, esi   
	add       edx, esi
	dec       edi
	jg        near d2top11


d2exit:
	;; Put the final sum in eax for return...
	movd	  eax, mm5
	psrlq	  mm5, 32
	movd	  ecx, mm5
    add       eax, ecx

	pop edi
	pop esi
	pop edx
	pop ecx
	pop ebx

	pop ebp			; restore stack pointer

	emms			; clear mmx registers
	ret	


; total squared difference between two (8*h) blocks
; blk1,blk2: addresses of top left pels of both blocks
; lx:        distance (in bytes) of vertically adjacent pels
; h:         height of block (usually 4, or 8)
; mmX version

global dist2_22_mmx
; int dist2_22_mmx(unsigned char *blk1, unsigned char *blk2,
;                 int lx, int h)

; mm7 = 0

; eax = pblk1 
; ebx = pblk2
; ecx = temp
; edx = distance_sum
; edi = h
; esi = lx

align 32
dist2_22_mmx:
	push ebp			; save frame pointer
	mov ebp, esp		; link
	push ebx
	push ecx
	push edx
	push esi     
	push edi

	mov		esi, [ebp+16] ; lx
	mov     edi, [ebp+20] ; h

    pxor      mm5, mm5      ; sum
	test      edi, edi     ; h = 0?
	jle       near d2exit

	pxor	  mm7, mm7     ; get zeros i mm7

	mov       eax, [ebp+8]		; blk1
    mov       ebx, [ebp+12]		; blk2
	jmp 	 d2top22

align 32
d2top22:
        movq      mm0, [eax]
        movq      mm1, mm0
        punpcklbw mm0, mm7
        punpckhbw mm1, mm7

        movq      mm2, [ebx]
        movq      mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

        psubw     mm0, mm2
        psubw     mm1, mm3
        pmaddwd   mm0, mm0
        pmaddwd   mm1, mm1
        paddd     mm5, mm0
		paddd    mm5, mm1

			add       eax, esi
	add       ebx, esi
	dec       edi
	jg        d2top22
	jmp       d2exit


; total squared difference between interpolation of two (8*h) blocks and
; another 8*h block		
; blk1,blk2: addresses of top left pels of both blocks
; lx:        distance (in bytes) of vertically adjacent pels
; h:         height of block (usually 4, or 8)
; mmX version
		
global bdist2_22_mmx
; int bdist2_22_mmx(unsigned char *blk1f, unsigned char*blk1b,
;				   unsigned char *blk2,
;                 int lx, int h)

; mm7 = 0

; eax = pblk1f 
; ebx = pblk2
; ecx = pblk1b
; edx = distance_sum
; edi = h
; esi = lx

align 32
bdist2_22_mmx:
	push ebp			; save frame pointer
	mov ebp, esp		; link
	push ebx
	push ecx
	push edx
	push esi     
	push edi

	mov		esi, [ebp+20] ; lx
	mov     edi, [ebp+24] ; h

    pxor      mm5, mm5      ; sum
	test      edi, edi     ; h = 0?
	jle       near d2exit

	pxor	  mm7, mm7     ; get zeros i mm7

	mov       eax, [ebp+8]		; blk1f
    mov       ebx, [ebp+12]		; blk1b
    mov       ecx, [ebp+16]		; blk2		
	jmp 	 bd2top22

align 32
bd2top22:
        movq      mm0, [eax]
        movq      mm1, mm0
		movq      mm4, [ebx]
		movq      mm6, mm4
        punpcklbw mm0, mm7
        punpckhbw mm1, mm7
		punpcklbw mm4, mm7
		punpckhbw mm6, mm7

        movq      mm2, [ecx]
        movq      mm3, mm2
        punpcklbw mm2, mm7
        punpckhbw mm3, mm7

		paddw	  mm0, mm4
		psrlw     mm0, 1
        psubw     mm0, mm2
        pmaddwd   mm0, mm0
		paddw     mm1, mm6
		psrlw	  mm1, 1
        psubw     mm1, mm3
        pmaddwd   mm1, mm1
        paddd     mm5, mm0
		paddd    mm5, mm1

		add       eax, esi
		add       ebx, esi
		add		  ecx, esi
		dec       edi
		jg        bd2top22
		jmp       d2exit
				