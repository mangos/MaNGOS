.686P
.MODEL FLAT
ASSUME FS: NOTHING
.CODE

extrn _memset:PROC

;
; int starcraft_compress_sparse(char * pbOutBuffer, int * pcbOutLength, char * pbInBuffer, int cbInBuffer);
;

_starcraft_compress_sparse PROC
var_C           = dword ptr -0Ch
var_8           = dword ptr -8
pbOutBufferEnd  = dword ptr -4
pbOutBuffer     = dword ptr  8
pcbOutBuffer    = dword ptr  0Ch
pbInBuffer      = dword ptr  10h
cbInBuffer      = dword ptr  14h

                push    ebp
                mov     ebp, esp
                sub     esp, 0Ch
                mov     eax, [ebp+pcbOutBuffer]
                mov     eax, [eax]
                mov     edx, [ebp+pbOutBuffer]
                add     eax, edx
                mov     [ebp+pbOutBufferEnd], eax
                mov     eax, [ebp+cbInBuffer]
                push    edi
                mov     edi, [ebp+pbInBuffer]
                add     edx, 4
                cmp     edx, [ebp+pbOutBufferEnd]
                lea     ecx, [edi+eax]
                mov     [ebp+var_C], ecx
                jnb     loc_466A24
                push    ebx
                push    esi
                mov     esi, [ebp+pbOutBuffer]
                mov     edx, eax
                shr     edx, 18h
                mov     [esi], dl
                inc     esi
                mov     edx, eax
                shr     edx, 10h
                mov     [esi], dl
                inc     esi
                mov     edx, eax
                shr     edx, 8
                mov     [esi], dl
                inc     esi
                mov     [esi], al
                lea     eax, [ecx-3]
                inc     esi
                cmp     edi, eax
                jnb     loc_4669FC
                mov     ebx, 80h

loc_466885:                             ; CODE XREF: Compress_sparse+1CDj
                and     [ebp+pbInBuffer], 0
                cmp     edi, ecx
                mov     eax, edi
                mov     [ebp+cbInBuffer], edi
                jnb     short loc_4668B5

loc_466892:                             ; CODE XREF: Compress_sparse+8Aj
                mov     edx, [ebp+cbInBuffer]
                cmp     byte ptr [edx], 0
                jnz     short loc_46689F
                inc     [ebp+pbInBuffer]
                jmp     short loc_4668AD
; ---------------------------------------------------------------------------

loc_46689F:                             ; CODE XREF: Compress_sparse+6Fj
                cmp     [ebp+pbInBuffer], 3
                jnb     short loc_4668B5
                mov     eax, [ebp+cbInBuffer]
                inc     eax
                and     [ebp+pbInBuffer], 0

loc_4668AD:                             ; CODE XREF: Compress_sparse+74j
                inc     [ebp+cbInBuffer]
                cmp     [ebp+cbInBuffer], ecx
                jb      short loc_466892

loc_4668B5:                             ; CODE XREF: Compress_sparse+67j
                                        ; Compress_sparse+7Aj
                sub     eax, edi
                mov     [ebp+cbInBuffer], eax
                jz      loc_466984
                cmp     eax, 81h
                jbe     short loc_466901
                lea     eax, [esi+81h]
                jmp     short loc_4668D2
; ---------------------------------------------------------------------------

loc_4668CF:                             ; CODE XREF: Compress_sparse+D6j
                mov     eax, [ebp+var_8]

loc_4668D2:                             ; CODE XREF: Compress_sparse+A4j
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     loc_466A22
                push    ebx
                mov     byte ptr [esi], 0FFh
                inc     esi
                inc     eax
                push    edi
                push    esi
                mov     [ebp+var_8], eax
                call    _memset        ; Microsoft VisualC 2-8/net runtime
                sub     [ebp+cbInBuffer], ebx
                add     [ebp+var_8], ebx
                add     esp, 0Ch
                add     esi, ebx
                add     edi, ebx
                cmp     [ebp+cbInBuffer], 81h
                ja      short loc_4668CF

loc_466901:                             ; CODE XREF: Compress_sparse+9Cj
                cmp     [ebp+cbInBuffer], ebx
                jbe     short loc_466926
                lea     eax, [esi+2]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     loc_466A22
                push    1
                mov     [esi], bl
                inc     esi
                push    edi
                push    esi
                call    _memset         ; Microsoft VisualC 2-8/net runtime
                add     esp, 0Ch
                inc     esi
                inc     edi
                dec     [ebp+cbInBuffer]

loc_466926:                             ; CODE XREF: Compress_sparse+DBj
                mov     eax, [ebp+cbInBuffer]
                cmp     eax, 1
                jb      short loc_466957
                lea     ecx, [eax+esi+1]
                cmp     ecx, [ebp+pbOutBufferEnd]
                jnb     loc_466A22
                mov     cl, al
                dec     cl
                or      cl, bl
                push    eax
                mov     [esi], cl
                inc     esi
                push    edi
                push    esi
                call    _memset         ; Microsoft VisualC 2-8/net runtime
                add     esi, [ebp+cbInBuffer]
                add     esp, 0Ch
                add     edi, [ebp+cbInBuffer]
                jmp     short loc_466981
; ---------------------------------------------------------------------------

loc_466957:                             ; CODE XREF: Compress_sparse+103j
                lea     eax, [esi+2]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     loc_466A22
                push    1
                mov     [esi], bl
                inc     esi
                push    edi
                push    esi
                call    _memset         ; Microsoft VisualC 2-8/net runtime
                mov     eax, [ebp+pbInBuffer]
                mov     ecx, [ebp+cbInBuffer]
                add     esp, 0Ch
                lea     eax, [eax+ecx-1]
                inc     esi
                inc     edi
                mov     [ebp+pbInBuffer], eax

loc_466981:                             ; CODE XREF: Compress_sparse+12Cj
                mov     ecx, [ebp+var_C]

loc_466984:                             ; CODE XREF: Compress_sparse+91j
                cmp     [ebp+pbInBuffer], 85h
                jbe     short loc_4669B9
                lea     eax, [esi+1]
                mov     [ebp+cbInBuffer], eax

loc_466993:                             ; CODE XREF: Compress_sparse+18Ej
                mov     eax, [ebp+cbInBuffer]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     loc_466A22
                mov     eax, 82h
                sub     [ebp+pbInBuffer], eax
                mov     byte ptr [esi], 7Fh
                inc     esi
                inc     [ebp+cbInBuffer]
                add     edi, eax
                cmp     [ebp+pbInBuffer], 85h
                ja      short loc_466993

loc_4669B9:                             ; CODE XREF: Compress_sparse+162j
                cmp     [ebp+pbInBuffer], 82h
                jbe     short loc_4669D6
                lea     eax, [esi+1]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     short loc_466A22
                mov     byte ptr [esi], 0
                add     edi, 3
                sub     [ebp+pbInBuffer], 3
                mov     esi, eax

loc_4669D6:                             ; CODE XREF: Compress_sparse+197j
                cmp     [ebp+pbInBuffer], 3
                jb      short loc_4669F1
                lea     eax, [esi+1]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     short loc_466A22
                mov     dl, byte ptr [ebp+pbInBuffer]
                sub     dl, 3
                add     edi, [ebp+pbInBuffer]
                mov     [esi], dl
                mov     esi, eax

loc_4669F1:                             ; CODE XREF: Compress_sparse+1B1j
                lea     eax, [ecx-3]
                cmp     edi, eax
                jb      loc_466885

loc_4669FC:                             ; CODE XREF: Compress_sparse+51j
                cmp     edi, ecx
                jnb     short loc_466A1A
                mov     eax, edi

loc_466A02:                             ; CODE XREF: Compress_sparse+1E2j
                mov     dl, [eax]
                inc     eax
                test    dl, dl
                jnz     short loc_466A27
                cmp     eax, ecx
                jb      short loc_466A02
                lea     eax, [esi+1]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     short loc_466A22
                mov     byte ptr [esi], 7Fh
                mov     esi, eax

loc_466A1A:                             ; CODE XREF: Compress_sparse+1D5j
                                        ; Compress_sparse+21Cj
                sub     esi, [ebp+pbOutBuffer]
                mov     eax, [ebp+pcbOutBuffer]
                mov     [eax], esi

loc_466A22:                             ; CODE XREF: Compress_sparse+ACj
                                        ; Compress_sparse+E3j ...
                pop     esi
                pop     ebx

loc_466A24:                             ; CODE XREF: Compress_sparse+26j
                pop     edi
                leave
                retn
; ---------------------------------------------------------------------------

loc_466A27:                             ; CODE XREF: Compress_sparse+1DEj
                sub     ecx, edi
                mov     ebx, ecx
                lea     eax, [ebx+esi+1]
                cmp     eax, [ebp+pbOutBufferEnd]
                jnb     short loc_466A22
                push    ebx
                mov     byte ptr [esi], 0FFh
                inc     esi
                push    edi
                push    esi
                call    _memset         ; Microsoft VisualC 2-8/net runtime
                add     esp, 0Ch
                add     esi, ebx
                jmp     short loc_466A1A
_starcraft_compress_sparse ENDP

;
; int starcraft_decompress_sparse(char * pbOutBuffer, int * pcbOutLength, char * pbInBuffer, int cbInBuffer);
;

_starcraft_decompress_sparse PROC
pbInBufferEnd   = dword ptr -4
pbOutBuffer     = dword ptr  8
pcbOutBuffer    = dword ptr  0Ch
pbInBuffer      = dword ptr  10h
cbInBuffer      = dword ptr  14h

                push    ebp
                mov     ebp, esp
                push    ecx
                mov     eax, [ebp+pbOutBuffer]
                push    esi
                mov     esi, [ebp+pbInBuffer] ; ESI - pbInBuffer
                mov     [ebp+pbOutBuffer], eax
                mov     eax, [ebp+cbInBuffer]
                cmp     eax, 5
                lea     ecx, [esi+eax]
                mov     [ebp+pbInBufferEnd], ecx
                jnb     short loc_466A6A
                xor     al, al
                jmp     loc_466AF5
; ---------------------------------------------------------------------------

loc_466A6A:                             ; CODE XREF: Decompress_sparse+1Aj
                push    edi
                movzx   edi, byte ptr [esi]
                shl     edi, 18h
                inc     esi
                movzx   eax, byte ptr [esi]
                shl     eax, 10h
                or      edi, eax
                xor     eax, eax
                inc     esi
                mov     ah, [esi]
                or      edi, eax
                inc     esi
                movzx   eax, byte ptr [esi]
                or      edi, eax
                mov     eax, [ebp+pcbOutBuffer]
                inc     esi
                cmp     esi, ecx
                mov     [ebp+pbInBuffer], edi ; EDI = cbOutBuffer
                mov     [eax], edi
                jnb     short loc_466AF2
                push    ebx

loc_466A95:                             ; CODE XREF: Decompress_sparse+A8j
                movzx   eax, byte ptr [esi]
                inc     esi
                test    al, al
                jns     short loc_466AC1
                and     eax, 7Fh
                inc     eax             ; AL = (*pbInBuffer & 0x7F) + 1
                mov     [ebp+cbInBuffer], eax
                cmp     eax, edi        ; AL < cbOutBuffer ?
                lea     eax, [ebp+cbInBuffer]
                jb      short loc_466AAE
                lea     eax, [ebp+pbInBuffer]

loc_466AAE:                             ; CODE XREF: Decompress_sparse+62j
                mov     ebx, [eax]
                push    ebx
                push    esi
                push    [ebp+pbOutBuffer]
                call    _memset         ; Microsoft VisualC 2-8/net runtime
                add     esp, 0Ch
                add     esi, ebx
                jmp     short loc_466AE4
; ---------------------------------------------------------------------------

loc_466AC1:                             ; CODE XREF: Decompress_sparse+54j
                and     eax, 7Fh
                add     eax, 3
                mov     [ebp+cbInBuffer], eax
                cmp     eax, edi
                lea     eax, [ebp+cbInBuffer]
                jb      short loc_466AD4
                lea     eax, [ebp+pbInBuffer]

loc_466AD4:                             ; CODE XREF: Decompress_sparse+88j
                mov     ebx, [eax]
                push    ebx             ; Size
                push    0               ; Val
                push    [ebp+pbOutBuffer] ; Dst
                call    _memset
                add     esp, 0Ch

loc_466AE4:                             ; CODE XREF: Decompress_sparse+78j
                add     [ebp+pbOutBuffer], ebx
                sub     edi, ebx
                cmp     esi, [ebp+pbInBufferEnd]
                mov     [ebp+pbInBuffer], edi
                jb      short loc_466A95
                pop     ebx

loc_466AF2:                             ; CODE XREF: Decompress_sparse+4Bj
                mov     al, 1
                pop     edi

loc_466AF5:                             ; CODE XREF: Decompress_sparse+1Ej
                pop     esi
                leave
                retn
_starcraft_decompress_sparse ENDP

END
