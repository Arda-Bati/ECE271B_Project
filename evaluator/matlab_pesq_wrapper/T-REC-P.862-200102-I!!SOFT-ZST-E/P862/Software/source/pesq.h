/*****************************************************************************

Perceptual Evaluation of Speech Quality (PESQ)
ITU-T Recommendation P.862.
Version 1.2 - 2 August 2002.

              ****************************************
              PESQ Intellectual Property Rights Notice
              ****************************************

DEFINITIONS:
------------
For the purposes of this Intellectual Property Rights Notice
the terms �Perceptual Evaluation of Speech Quality Algorithm�
and �PESQ Algorithm� refer to the objective speech quality
measurement algorithm defined in ITU-T Recommendation P.862;
the term �PESQ Software� refers to the C-code component of P.862. 

NOTICE:
-------
All copyright, trade marks, trade names, patents, know-how and
all or any other intellectual rights subsisting in or used in
connection with including all algorithms, documents and manuals
relating to the PESQ Algorithm and or PESQ Software are and remain
the sole property in law, ownership, regulations, treaties and
patent rights of the Owners identified below. The user may not
dispute or question the ownership of the PESQ Algorithm and
or PESQ Software.

OWNERS ARE:
-----------

1.	British Telecommunications plc (BT), all rights assigned
      to Psytechnics Limited
2.	Royal KPN NV, all rights assigned to OPTICOM GmbH

RESTRICTIONS:
-------------

The user cannot:

1.	alter, duplicate, modify, adapt, or translate in whole or in
      part any aspect of the PESQ Algorithm and or PESQ Software
2.	sell, hire, loan, distribute, dispose or put to any commercial
      use other than those permitted below in whole or in part any
      aspect of the PESQ Algorithm and or PESQ Software

PERMITTED USE:
--------------

The user may:

1.	Use the PESQ Software to:
      i)   understand the PESQ Algorithm; or
      ii)  evaluate the ability of the PESQ Algorithm to perform
           its intended function of predicting the speech quality
           of a system; or
      iii) evaluate the computational complexity of the PESQ Algorithm,
           with the limitation that none of said evaluations or its
           results shall be used for external commercial use.

2.	Use the PESQ Software to test if an implementation of the PESQ
      Algorithm conforms to ITU-T Recommendation P.862.

3.	With the prior written permission of both Psytechnics Limited
      and OPTICOM GmbH, use the PESQ Software in accordance with the
      above Restrictions to perform work that meets all of the following
      criteria:
      i)    the work must contribute directly to the maintenance of an
            existing ITU recommendation or the development of a new ITU
            recommendation under an approved ITU Study Item; and
      ii)   the work and its results must be fully described in a
            written contribution to the ITU that is presented at a formal
            ITU meeting within one year of the start of the work; and
      iii)  neither the work nor its results shall be put to any
            commercial use other than making said contribution to the ITU.
            Said permission will be provided on a case-by-case basis.


ANY OTHER USE OR APPLICATION OF THE PESQ SOFTWARE AND/OR THE PESQ
ALGORITHM WILL REQUIRE A PESQ LICENCE AGREEMENT, WHICH MAY BE OBTAINED
FROM EITHER OPTICOM GMBH OR PSYTECHNICS LIMITED. 

EACH COMPANY OFFERS OEM LICENSE AGREEMENTS, WHICH COMBINE OEM
IMPLEMENTATIONS OF THE PESQ ALGORITHM TOGETHER WITH A PESQ PATENT LICENSE
AGREEMENT. PESQ PATENT-ONLY LICENSE AGREEMENTS MAY BE OBTAINED FROM OPTICOM.


***********************************************************************
*  OPTICOM GmbH                    *  Psytechnics Limited             *
*  Am Weichselgarten 7,            *  Fraser House, 23 Museum Street, *
*  D- 91058 Erlangen, Germany      *  Ipswich IP1 1HN, England        *
*  Phone: +49 (0) 9131 691 160     *  Phone: +44 (0) 1473 261 800     *
*  Fax:   +49 (0) 9131 691 325     *  Fax:   +44 (0) 1473 261 880     *
*  E-mail: info@opticom.de,        *  E-mail: info@psytechnics.com,   *
*  www.opticom.de                  *  www.psytechnics.com             *
***********************************************************************

Further information is also available from www.pesq.org

*****************************************************************************/

#include <string.h>
#include <stdlib.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#define LINIIR 60

#define MAXNUTTERANCES 50

#define WHOLE_SIGNAL -1

#define LINIIR 60
#define LSMJ 20
#define LFBANK 35

#define DATAPADDING_MSECS 320
#define SEARCHBUFFER 75  

#define EPS 1E-12

#define MINSPEECHLGTH 4

#define JOINSPEECHLGTH 50

#define MINUTTLENGTH 50

#define SATDB 90.31
#define FIXDB -32.0

#define TWOPI 6.28318530717959

int Nb ;

#define Nfmax 512

#define Sp_8k   2.764344e-5
#define Sl_8k   1.866055e-1

#define Sp_16k  6.910853e-006
#define Sl_16k  1.866055e-001

extern float Sp;
extern float Sl;

#define Dz 0.312

#define gamma 0.001

#define Tl 10000.0f

#define Ts 10000000.0f

#define Tt 0.02f

#define Tn 0.01f


#ifndef min
  #define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
  #define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif


typedef struct {
  char  path_name[512];
  char  file_name [128];
  long  Nsamples;
  long  apply_swap;

  float * data;
  float * VAD;
  float * logVAD;
} SIGNAL_INFO;

typedef struct {
  long Nutterances;
  long Largest_uttsize;
  long Nsurf_samples;

  long  Crude_DelayEst;
  float Crude_DelayConf;
  long  UttSearch_Start[MAXNUTTERANCES];
  long  UttSearch_End[MAXNUTTERANCES];
  long  Utt_DelayEst[MAXNUTTERANCES];
  long  Utt_Delay[MAXNUTTERANCES];
  float Utt_DelayConf[MAXNUTTERANCES];
  long  Utt_Start[MAXNUTTERANCES];
  long  Utt_End[MAXNUTTERANCES];

  float pesq_mos;
  float subj_mos;
  int   cond_nr;
} ERROR_INFO;


extern long Fs;
extern long Downsample;
extern float * InIIR_Hsos;
extern long Align_Nfft;

extern long Fs_8k;
extern long Downsample_8k;
extern long InIIR_Nsos_8k;
extern long Align_Nfft_8k;

extern long Fs_16k;
extern long Downsample_16k;
extern long InIIR_Nsos_16k;
extern long Align_Nfft_16k;

extern float * InIIR_Hsos;

void input_filter(
       SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info, float * ftmp );
void apply_filters( float * data, long Nsamples );
void make_stereo_file (char *, SIGNAL_INFO *, SIGNAL_INFO *);
void make_stereo_file2 (char *, SIGNAL_INFO *, float *);
void select_rate( long sample_rate,
     long * Error_Flag, char ** Error_Type );
int  file_exist( char * fname );
void load_src( long * Error_Flag, char ** Error_Type,
     SIGNAL_INFO * sinfo);
void alloc_other( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info, 
    long * Error_Flag, char ** Error_Type, float ** ftmp);
void calc_VAD( SIGNAL_INFO * pinfo );
int  id_searchwindows( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info );
void id_utterances( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info );
void utterance_split( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info, float * ftmp );
void utterance_locate( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info, float * ftmp );
void auditory_transform( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info, long Utt_id, float * ftmp);
void calc_err( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info, 
       ERROR_INFO * err_info, long Utt_id);
void extract_params( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
    ERROR_INFO * err_info, long Utt_id, float * ftmp );
void utterance_process(SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
       ERROR_INFO * err_info, long Utt_id, float * ftmp);
void DC_block( float * data, long Nsamples );
void apply_filter ( float * data, long Nsamples, int, double [][2] );
double pow_of (const float * const , long , long, long);
void apply_VAD(
     SIGNAL_INFO * pinfo, float * data, float * VAD, float * logVAD );
void crude_align(
     SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info, ERROR_INFO * err_info,
     long Utt_id, float * ftmp);
void time_align(
     SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info, ERROR_INFO * err_info,
     long Utt_id, float * ftmp );
void split_align( SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
     ERROR_INFO * err_info, float * ftmp,
     long Utt_Start, long Utt_SpeechStart, long Utt_SpeechEnd, long Utt_End,
     long Utt_DelayEst, float Utt_DelayConf,
     long * Best_ED1, long * Best_D1, float * Best_DC1,
     long * Best_ED2, long * Best_D2, float * Best_DC2,
     long * Best_BP );
void pesq_psychoacoustic_model(
SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
ERROR_INFO * err_info, float * ftmp);
void apply_pesq( float * x_data, float * ref_surf,
float * y_data, float * deg_surf, long NVAD_windows, float * ftmp,
ERROR_INFO * err_info );



#define     D_POW_F     2
#define     D_POW_S     6
#define     D_POW_T     2

#define     A_POW_F     1
#define     A_POW_S     6
#define     A_POW_T     2


#define     D_WEIGHT    0.1
#define     A_WEIGHT    0.0309

/* END OF FILE */

