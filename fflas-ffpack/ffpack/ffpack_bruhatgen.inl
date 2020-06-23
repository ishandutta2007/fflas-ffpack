/* ffpack/ffpack_bruhatgen.inl
 * Copyright (C) 2020 Quentin Houssier
 *
 * Written by Quentin Houssier <quentin.houssier@ecl18.ec-lyon.fr>
 *
 *
 * ========LICENCE========
 * This file is part of the library FFLAS-FFPACK.
 *
 * FFLAS-FFPACK is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 *.
 */


#ifndef __FFLASFFPACK_ffpack_bruhatgen_inl
#define __FFLASFFPACK_ffpack_bruhatgen_inl

namespace FFPACK{
template<class Field>
inline size_t LTBruhatGen (const Field& Fi, const FFLAS::FFLAS_DIAG diag,
            const size_t N,
           typename Field::Element_ptr A, const size_t lda,
           size_t * P, size_t * Q)
    {
        if (N == 1) {Fi.assign(*(A+0), Fi.zero);
        return(0);}
        
        size_t N2 = N >> 1; // N = colonnes
        FFLAS::FFLAS_DIAG OppDiag =(diag==FFLAS::FflasUnit)?FFLAS::FflasNonUnit : FFLAS::FflasUnit;
        size_t * P1 = FFLAS::fflas_new<size_t>(N-N2);
        size_t * Q1 = FFLAS::fflas_new<size_t>(N2);
        // A1 = P1 [ L1 ] [ U1 V1 ] Q1
        //         [ M1 ]
        size_t r1 = PLUQ (Fi, diag, N-N2, N2, A, lda, P1, Q1);
        LAPACKPerm2MathPerm (P, P1, N-N2);
        LAPACKPerm2MathPerm (Q, Q1, N2);
        typename Field::Element_ptr A2 = A + N2;
        typename Field::Element_ptr A3 = A + (N-N2)*lda;
        typename Field::Element_ptr F = A2 + r1*lda;
        typename Field::Element_ptr G = A3 + r1;
        // [ B1 ] <- P1^T A2
        // [ B2 ]
        applyP (Fi, FFLAS::FflasLeft, FFLAS::FflasNoTrans, N-N2, size_t(0), N-N2, A2, lda, P1);
        // [ C1 C2 ] <- A3 Q1^T
        applyP (Fi, FFLAS::FflasRight, FFLAS::FflasTrans, N2, size_t(0), N2, A3, lda, Q1);

        // D <- L1^-1 B1
        ftrsm (Fi, FFLAS::FflasLeft, FFLAS::FflasLower, FFLAS::FflasNoTrans, OppDiag, r1, N-N2, Fi.one, A, lda, A2, lda);
        // E <- C1 U1^-1
        ftrsm (Fi, FFLAS::FflasRight, FFLAS::FflasUpper, FFLAS::FflasNoTrans, diag, N2, r1, Fi.one, A, lda, A3, lda);
        // F <- B2 - M1 D
        fgemm (Fi, FFLAS::FflasNoTrans, FFLAS::FflasNoTrans, N-N2-r1, N-N2, r1, Fi.mOne, A + r1*lda, lda, A2, lda, Fi.one, A2+r1*lda, lda);
        // G <- C2 - E V1
        fgemm (Fi, FFLAS::FflasNoTrans, FFLAS::FflasNoTrans, N2, N2-r1, r1, Fi.mOne, A3, lda, A+r1, lda, Fi.one, A3+r1, lda);
        // Expand L1\U1 into Bruhat generator
        applyP(Fi,FFLAS::FflasLeft,FFLAS::FflasTrans, N2, size_t(0), N-N2, A, lda, P1);
        applyP(Fi, FFLAS::FflasRight,FFLAS::FflasNoTrans, N-N2,size_t(0), N2, A, lda, Q1);
        //On stock D et E
        typename Field::Element_ptr D= FFLAS::fflas_new(Fi, r1, N-N2); 
        FFLAS::fassign(Fi, r1,N-N2, A2, lda, D, N-N2);
        size_t ldd = N-N2;
        typename Field::Element_ptr E= FFLAS::fflas_new(Fi, N2, r1); 
        FFLAS::fassign(Fi, N2,r1, A3, lda, E, r1);
        size_t lde = r1;
        FFLAS::fzero(Fi, r1, N-N2, A2,lda);
        FFLAS::fzero(Fi, N2, r1, A3, lda);
        //H <- P1  [ 0 ]
        //         [ F ]
        applyP (Fi, FFLAS::FflasLeft, FFLAS::FflasTrans, N-N2, size_t(0),N-N2, A2, lda, P1);
        //I <- [ 0 G ] Q1
        applyP (Fi, FFLAS::FflasRight, FFLAS::FflasNoTrans, N2, size_t(0),N2, A3, lda, Q1);
        FFLAS::fflas_delete(P1,Q1);
        //A2 <- LT-Bruhat(H)
        
        size_t r2 = LTBruhatGen(Fi,diag,N-N2, A2,lda, P+r1, Q+r1);
        for (size_t i=r1;i<r1+r2;i++) Q[i]+=N2;
        //Restore raws of D into their position in A2
        for (size_t i=0;i<r1;i++){
            size_t row = P[i];
            FFLAS::fassign (Fi, N-N2-1-row, D+i*ldd, 1, A2+row*lda, 1);
        }
        FFLAS::fflas_delete(D);
        //A3 <- LT-Bruhat(I)
        size_t r3 = LTBruhatGen (Fi,diag, N2, A3, lda, P+r1+r2, Q+r1+r2);
        for (size_t i=r1+r2;i<r1+r2+r3;i++) P[i]+=N-N2;
        for (size_t i=0;i<r1;i++){
            size_t col = Q[i];
            FFLAS::fassign(Fi, N2-1-col, E+i, lde, A3+col,lda);
        }
        FFLAS::fflas_delete(E);
    return(r1+r2+r3);		
    
    }

template<class Field>
inline void getLTBruhatGen(const Field& Fi, const size_t N, const size_t r,const size_t * P, const size_t * Q, typename Field::Element_ptr R, const size_t ldr)
{
  FFLAS::fzero(Fi, N, N, R,ldr);
    for(size_t i=0;i<r;i++){
            size_t row = P[i];
            size_t col = Q[i];
            Fi.assign(R[row*ldr+col],Fi.one);
        }
    
}
template<class Field>
inline void getLTBruhatGen(const Field& Fi, const FFLAS::FFLAS_UPLO Uplo,const FFLAS::FFLAS_DIAG diag,
                           const size_t N, const size_t r, const size_t *P, const size_t * Q,
                           typename Field::ConstElement_ptr A, const size_t lda,
                           typename Field::Element_ptr T, const size_t ldt)

{   FFLAS::fzero(Fi, N, N, T, N);
    //U
    if (Uplo==FFLAS::FflasUpper) {
      if(diag==FFLAS::FflasNonUnit){
          for(size_t i=0; i<r;i++){
            size_t row = P[i];
            size_t col = Q[i];
            FFLAS::fassign(Fi, N-1-row-col, A+row*lda+col,1,T+row*ldt+col,1);
            for(size_t j=0;j<i;j++){
                Fi.assign(T[row*ldt+Q[j]],Fi.zero);
            }
          }
      }
      else
        {
          for(size_t i=0; i<r;i++){
            size_t row = P[i];
            size_t col = Q[i];
            FFLAS::fassign(Fi, N-2-row-col, A+row*lda+col+1,1,T+row*ldt+col+1,1);
            Fi.assign(T[row*ldt+col],Fi.one);
            for(size_t j=0;j<i;j++){
              Fi.assign(T[row*ldt+Q[j]],Fi.zero);
            }
          }

        }
    }
    //L
    else
      {
        if(diag==FFLAS::FflasNonUnit){

          for(size_t i=0; i<r;i++){
            size_t row = P[i];
            size_t col = Q[i];

            FFLAS::fassign(Fi, N-1-row-col, A+row*lda+col,lda,T+row*ldt+col,ldt);
            for(size_t j=0;j<i;j++){
                Fi.assign(T[P[j]*ldt+col],Fi.zero);
            }
          
          }
        }

        else
          {
            for(size_t i=0; i<r;i++){
              size_t row = P[i];
              size_t col = Q[i];
              FFLAS::fassign(Fi, N-2-row-col, A+(row+1)*lda+col,lda,T+(row+1)*ldt+col,ldt);
              Fi.assign(T[row*ldt+col],Fi.one);
              for(size_t j=0;j<i;j++){
                Fi.assign(T[P[j]*ldt+col],Fi.zero);
              }
          
            }
          }
      }
 
}
    

inline size_t LTQSorder(const size_t N, const size_t r,const size_t * P, const size_t * Q){
    std::vector<bool> rows(N,false);
    std::vector<bool> cols(N,false);
    
    for (size_t i=0;i<r;i++)
    {
        rows[P[i]]=true;
        cols[Q[i]]=true;
    }
    size_t s=0;
    size_t t=0;
    if (rows[0])
    {
        s=1;
        t=1;
    }

    for (size_t i=1;i<N;i++)
    {
        if (rows[i])   t+=1;
        if (cols[N-1-i]) t-=1;
        s = std::max(s,t);
    }
    return(s);
}

template<class Field>
inline void CompressToBlockBiDiagonal(const Field&Fi,size_t N, size_t s, size_t r, const size_t *P, const size_t *Q,  typename Field::Element_ptr A, size_t lda, Field::Element_ptr X, size_t ldx){
  std::vector<bool> rows(N,false);
  std::vector<bool> cols(N,false);
    
  for (size_t i=0;i<r;i++)
    {
      rows[P[i]]=true;
      cols[Q[i]]=true;
    }
  int k = N/s;
  int m =N%s;
  for (size_t i=1;i<k;i++)
    {
      FFLAS::fassign(Fi, s, s, C+((k-i)*s)*lda+i*s,lda,X+i*s,ldx);
      FFLAS::fzero(Fi, s, s, C+((k-i)*s)*lda+i*s,lda);
    }
  for (size_t i=0;i<k-1;i++)
    { for(size_t t=0;t<s;t++)
        {
          if(cols[i*s+t])
            {
              bool haschanged = false;
              for(size_t l=0;l<s;l++)
                {
                  if(!haschanged && !cols[(i+1)*s+l])
                    {
                      FFLAS::fassign(Fi,(k-1-i)*s , C+i*s+t,lda, C+(i+1)*s+l, lda)
                        haschanged = true;
                    }
                }
            }
        }
    }

      // Compute M such that LM is in column echelon form, where L is the
      // left factor of a Bruhat decomposition
      // M is allocated in this function and should be deleted after using it.
  size_t* void Bruhat2EchelonPermutation (N,R,P,Q){

      size_t * Pinv = FFLAS::fflas_new<size_t>(N);
      size_t * Ps = FFLAS::fflas_new<size_t>(R);
      size_t * M = FFLAS::fflas_new<size_t>(R);

      for (size_t i=0; i<R; i++){
          Pinv [P [i]] = i;
          Ps[i] = P[i];
      }
      std::sort (Ps, Ps+R);

      for (size_t i=0; i<R; i++)
          M[i] = Q [Pinv [Ps[i]]];
      return M;
  }
  



}
   
} //namespace FFPACK
#endif //_FFPACK_ffpack_bruhatgen_inl
