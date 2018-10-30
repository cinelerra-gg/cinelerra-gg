#ifndef _HUF_H_
#define _HUF_H_

class huf {
  static uint8_t
    huf100[],huf101[],huf102[],huf103[],huf104[],huf105[],huf106[],huf107[],
    huf108[],huf109[],huf10a[],huf10b[],huf10c[],huf10d[],huf10e[],huf10f[],
    huf110[],huf111[],huf112[],huf113[],huf114[],huf115[],huf116[],huf117[],
    huf118[],huf119[],huf11a[],huf11b[],huf11c[],huf11d[],huf11e[],huf11f[],
    huf120[],huf121[],huf122[],huf123[],huf124[],huf125[],huf126[],huf127[],
    huf128[],huf129[],huf12a[],huf12b[],huf12c[],huf12d[],huf12e[],huf12f[],
    huf130[],huf131[],huf132[],huf133[],huf134[],huf135[],huf136[],huf137[],
    huf138[],huf139[],huf13a[],huf13b[],huf13c[],huf13d[],huf13e[],huf13f[],
    huf140[],huf141[],huf142[],huf143[],huf144[],huf145[],huf146[],huf147[],
    huf148[],huf149[],huf14a[],huf14b[],huf14c[],huf14d[],huf14e[],huf14f[],
    huf150[],huf151[],huf152[],huf153[],huf154[],huf155[],huf156[],huf157[],
    huf158[],huf159[],huf15a[],huf15b[],huf15c[],huf15d[],huf15e[],huf15f[],
    huf160[],huf161[],huf162[],huf163[],huf164[],huf165[],huf166[],huf167[],
    huf168[],huf169[],huf16a[],huf16b[],huf16c[],huf16d[],huf16e[],huf16f[],
    huf170[],huf171[],huf172[],huf173[],huf174[],huf175[],huf176[],huf177[],
    huf178[],huf179[],huf17a[],huf17b[],huf17c[],huf17d[],huf17e[],huf17f[],
    *huf1[];
  static uint8_t
    huf200[],huf201[],huf202[],huf203[],huf204[],huf205[],huf206[],huf207[],
    huf208[],huf209[],huf20a[],huf20b[],huf20c[],huf20d[],huf20e[],huf20f[],
    huf210[],huf211[],huf212[],huf213[],huf214[],huf215[],huf216[],huf217[],
    huf218[],huf219[],huf21a[],huf21b[],huf21c[],huf21d[],huf21e[],huf21f[],
    huf220[],huf221[],huf222[],huf223[],huf224[],huf225[],huf226[],huf227[],
    huf228[],huf229[],huf22a[],huf22b[],huf22c[],huf22d[],huf22e[],huf22f[],
    huf230[],huf231[],huf232[],huf233[],huf234[],huf235[],huf236[],huf237[],
    huf238[],huf239[],huf23a[],huf23b[],huf23c[],huf23d[],huf23e[],huf23f[],
    huf240[],huf241[],huf242[],huf243[],huf244[],huf245[],huf246[],huf247[],
    huf248[],huf249[],huf24a[],huf24b[],huf24c[],huf24d[],huf24e[],huf24f[],
    huf250[],huf251[],huf252[],huf253[],huf254[],huf255[],huf256[],huf257[],
    huf258[],huf259[],huf25a[],huf25b[],huf25c[],huf25d[],huf25e[],huf25f[],
    huf260[],huf261[],huf262[],huf263[],huf264[],huf265[],huf266[],huf267[],
    huf268[],huf269[],huf26a[],huf26b[],huf26c[],huf26d[],huf26e[],huf26f[],
    huf270[],huf271[],huf272[],huf273[],huf274[],huf275[],huf276[],huf277[],
    huf278[],huf279[],huf27a[],huf27b[],huf27c[],huf27d[],huf27e[],huf27f[],
    *huf2[];
  static int huf_decode(uint8_t **tree,uint8_t *msg,int mlen,char *bfr,int blen);

public:
  static int huf1_decode(uint8_t *msg,int mlen,char *bfr,int blen) {
    return huf_decode(huf1,msg,mlen,bfr,blen);
  }
  static int huf2_decode(uint8_t *msg,int mlen,char *bfr,int blen) {
    return huf_decode(huf2,msg,mlen,bfr,blen);
  }
};

#endif
