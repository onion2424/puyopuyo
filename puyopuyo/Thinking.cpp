#include "Thinking.h"
#include <random>
#include <vector>
#include <iostream>
#include <immintrin.h>
#include <string>

int Thinking::RANGE = 400;

//patリセット
void Thinking::resetPat() {
	for (int i = 0; i < 22; i++) {
		pat[i] = 0;
	}
}

//配列からfieldにセット
void Thinking::setField(int nowField[8][16]) {
	unsigned long long tmpfield[6] = {};
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 16; j++) {
			(tmpfield[0] <<= 1) ^= (0b1 & nowField[i][j]);
			(tmpfield[2] <<= 1) ^= ((0b10 & nowField[i][j]) >> 1);
			(tmpfield[4] <<= 1) ^= ((0b100 & nowField[i][j]) >> 2);
		}
	}
	for (int i = 4; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			(tmpfield[1] <<= 1) ^= (0b1 & nowField[i][j]);
			(tmpfield[3] <<= 1) ^= ((0b10 & nowField[i][j]) >> 1);
			(tmpfield[5] <<= 1) ^= ((0b100 & nowField[i][j]) >> 2);
		}
	}
	field_real[1] = _mm_set_epi64x(tmpfield[0], tmpfield[1]);
	field_real[2] = _mm_set_epi64x(tmpfield[2], tmpfield[3]);
	field_real[3] = _mm_set_epi64x(tmpfield[4], tmpfield[5]);
	field_real[0] = _mm_or_si128(field_real[3], _mm_or_si128(field_real[2], field_real[1]));
	return;
}


//ツモをランダムに作成(４色128手均等)
void Thinking::makeTsumo() {
	for (int i = 0; i < 128; i++) {
		tsumo[i] = i / 32 + 1;
	}
	std::random_device get_rand_dev;
	std::mt19937 get_rand_mt(get_rand_dev()); // seedに乱数を指定
	std::shuffle(tsumo.begin(), tsumo.end(), get_rand_mt);
	//初手とその次は見えるツモに合わせる(ループよりは早い？)
	tsumo[0] = nextTsumo[0];
	tsumo[1] = nextTsumo[1];
	tsumo[2] = nextTsumo[2];
	tsumo[3] = nextTsumo[3];
	tsumo[4] = nextTsumo[4];
	tsumo[5] = nextTsumo[5];
}

//	高さを配列に格納
void Thinking::setHeight(__m128i reg, int array[]) {
	uint16_t* val = (uint16_t*)&reg;
	for (int i = 0; i < 6; i++) {
		array[i] = __popcnt16(val[6 - i]);
	}
	return;
}

//ツモをおける位置を取得(引数にセットして返す)
void Thinking::getFallPosition(int& start, int& end, int height[]) {
	//13段の壁は越えられない
	if (height[1] < 13) {
		if (height[0] < 13) {
			start = 0;
		}
		else {
			start = 3; //上のツモは14段目にのせられる
		}

	}
	else {
		start = 7; //上のツモは14段目にのせられる
	}

	if (height[3] < 13) {
		if (height[4] < 13) {
			if (height[5] < 13) {
				end = 21;
			}
			else {
				end = 18; //上のツモは14段目にのせられる
			}
		}
		else {
			end = 14; //上のツモは14段目にのせられる
		}
	}
	else {
		end = 10; //上のツモは14段目にのせられる
	}
	return;
}

//	次の手を置く
void Thinking::fallTsumo(int pat, int tsumo[]) {
	//	fallpoint <- (1, 1)の位置
	//	fallpoint1 <- 縦置き時の上側と横置き時の左側　０～３は１列目４～７は２列目
	//	fallpoint2 <- 縦置き時の下側と横置き時の右側　２～５は２列目６～１０は３列目
	__m128i fallPoint1, fallPoint2;
	switch (pat / 4) {
	case 0: fallPoint1 = fallPoint; break;
	case 1: fallPoint1 = _mm_srli_si128(fallPoint, 2); break;
	case 2: fallPoint1 = _mm_srli_si128(fallPoint, 4); break;
	case 3: fallPoint1 = _mm_srli_si128(fallPoint, 6); break;
	case 4: fallPoint1 = _mm_srli_si128(fallPoint, 8); break;
	case 5: fallPoint1 = _mm_srli_si128(fallPoint, 10); break;
	default: fallPoint1 = _mm_setzero_si128();
	}

	switch ((pat + 2) / 4) {
	case 0: fallPoint2 = fallPoint; break;
	case 1: fallPoint2 = _mm_srli_si128(fallPoint, 2); break;
	case 2: fallPoint2 = _mm_srli_si128(fallPoint, 4); break;
	case 3: fallPoint2 = _mm_srli_si128(fallPoint, 6); break;
	case 4: fallPoint2 = _mm_srli_si128(fallPoint, 8); break;
	case 5: fallPoint2 = _mm_srli_si128(fallPoint, 10); break;
	default: fallPoint2 = _mm_setzero_si128();
	}
	fallPoint1 = _mm_andnot_si128(vanish14, _mm_srli_epi16(fallPoint1, fieldHeight_image[pat / 4] + (((pat + 2) / 2) % 2))); //　(((pat + 2) / 2) % 2)) <= 上のときと下のときがあるので
	fallPoint2 = _mm_andnot_si128(vanish14, _mm_srli_epi16(fallPoint2, fieldHeight_image[(pat + 2) / 4]));
	//↑14段目は削除

	//1パターンずつ置く

	//　そのまま落下時の上が〇下が×とすると
	//　　０　１　２　　３
	//　　×　〇　
	//　　〇　×　×〇　〇×　にしたい
	//　という順で２２手置く
	//	fallpoint1, 2に対応する〇と×は1手ごとに入れかわることになる


	//	ツモの色に応じてfallPointを反映する
	//	field_imageがfield_cpと同じになっている前提

	//	1つめを置く
	field_image[tsumo[(pat + 1) % 2] & 0x1] = _mm_or_si128(field_image[tsumo[(pat + 1) % 2] & 0x1], fallPoint1);
	field_image[((tsumo[(pat + 1) % 2] >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((tsumo[(pat + 1) % 2] >> 1) & 0x1) * 2], fallPoint1);
	field_image[((tsumo[(pat + 1) % 2] >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((tsumo[(pat + 1) % 2] >> 2) & 0x1) * 3], fallPoint1);

	//	2つめを置く
	field_image[tsumo[pat % 2] & 0x1] = _mm_or_si128(field_image[tsumo[pat % 2] & 0x1], fallPoint2);
	field_image[((tsumo[pat % 2] >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((tsumo[pat % 2] >> 1) & 0x1) * 2], fallPoint2);
	field_image[((tsumo[pat % 2] >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((tsumo[pat % 2] >> 2) & 0x1) * 3], fallPoint2);

	//３フィールドのor(ツモ存在位置)を更新
	field_image[0] = _mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1]));

	return;

}

//	連鎖しているかを試す
int Thinking::tryChain() {

	//__m128i ojama = _mm_and_si128(*reg1, _mm_and_si128(*reg2, *reg3));
	__m128i colorReg = _mm_setzero_si128();
	__m128i tmpRegV;
	__m128i tmpRegH;
	__m128i sheedV;
	__m128i sheedH;
	__m128i linkOne;
	__m128i linkTwo;
	__m128i sheed;
	__m128i vanish = _mm_setzero_si128();
	__m128i pdepMask;
	__m128i pextMask;
	__m128i thirteen[3];
	//__m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13段目にビットが立っている
	//__m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);
	
	uint16_t* cnt = (uint16_t*)&vanish;
	uint64_t* pdepMaskPtr = (uint64_t*)&pdepMask;
	uint64_t* pextMaskPtr = (uint64_t*)&pextMask;
	uint64_t* fieldPtr1 = (uint64_t*)&field_image[1];
	uint64_t* fieldPtr2 = (uint64_t*)&field_image[2];
	uint64_t* fieldPtr3 = (uint64_t*)&field_image[3];
	uint64_t* vanishPtr = (uint64_t*)&vanish;

	//連鎖数を返す
	int count = 0;

	while (true) {
		//13段目の情報を保存
		//その後、フィールド(1～12段目)以外の情報を消す
		thirteen[0] = _mm_and_si128(vanish13, field_image[1]);
		thirteen[1] = _mm_and_si128(vanish13, field_image[2]);
		thirteen[2] = _mm_and_si128(vanish13, field_image[3]);
		field_image[1] = _mm_and_si128(vanishOther, field_image[1]);
		field_image[2] = _mm_and_si128(vanishOther, field_image[2]);
		field_image[3] = _mm_and_si128(vanishOther, field_image[3]);
		for (int color = 1; color < 5; color++) {
			//一色ずつ試す
			switch (color) {																						     //	    reg3   reg2   reg1	
			case 1: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[2], field_image[1])); break;//赤 :  0		0	   1
			case 2: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[1], field_image[2])); break;//緑 :  0      1      0
			case 3: colorReg = _mm_andnot_si128(field_image[3], _mm_and_si128(field_image[1], field_image[2])); break;//青 :     0      1      1
			case 4: colorReg = _mm_andnot_si128(field_image[2], _mm_andnot_si128(field_image[1], field_image[3])); break;//黄色  1      0      0
			//case 5: colorReg = _mm_andnot_si128(field_image[2], _mm_and_si128(field_image[1], field_image[3])); break; //紫    1      0      1
			}

			//1連結のツモを探す
			//  上下どちらかに同じ色があるツモが存在する位置にビットを立てる
			//	tmpRegV : 1つ上にずらしたフィールドとand　-> 1なら下にツモがあることになる
			//	sheedV	: tmpRegVのフィールドを下にずらしてxor -> 下のやつも1を立てる (左右にも繋がっているのは0に戻る)
			tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
			sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			//			↑xorは左右片方だけ繋がっている					
			//	左右どちらかに同じ色があるツモが存在する位置にビットを立てる
			//	tmpRegH : 1つ左にずらしたフィールドとand -> 1なら右にツモがあることになる
			//	sheedH	: tmpRegHのフィールドを左にずらしてxor -> 右のやつも1を立てる (左右にも繋がっているのは0に戻る)
			tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
			sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			//　連結1のツモ位置にビットを立てる
			//	左右どちらか　か　上下どちらか　だけビットを立てる
			linkOne = _mm_xor_si128(sheedV, sheedH);


			//  2連結のツモを探す
			//	2つ同じ色が連結しているツモが存在する位置にビットを立てる　（3つは除く） 条件は↓の3つ
			//						縦横両方繋がっている			or			横が繋がらず上下両方に繋がっているやつ										or					縦が繋がらず左右両方に繋がっているやつ				
			linkTwo = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
			//																								↑andは両方つながってる														↑andは両方つながってる


			//  3連結以上のツモを探す
			//	3つ以上同じ色が連結しているツモが存在する位置にビットを立てる																			
			//						4連結の条件→				（上下繋がっている　　　　　　　　　かつ　　　　　　　左右繋がっている　						or	→3連結の条件　(上下両方が繋がってる and   左右どちらかに繋がってる)				or		(左右両方に繋がっている   and  上下どちらかに繋がっている)															
			sheed = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128( _mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))) );
			//											↑andは両方つながってる								↑andは両方つながってる																	↑andは両方つながってる														↑andは両方つながってる
			 
			

			//	シード（消えるきっかけ）の位置にビットを立てる
			//	条件 = 連結3以上の奴 or 隣接の連結が2以上の連結2の奴
			//	
			//	連結2以上のやつ
			linkTwo = _mm_or_si128(sheed, linkTwo);
			//										   
			//	tmpRegV : 2連結以上が左右のどちらかあるいは両方繋がっているやつ
			tmpRegV = _mm_and_si128(linkTwo, _mm_slli_epi16(linkTwo, 1));
			tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			//			↑orは少なくとも片方は繋がってる
			//	tmpRegH : 2連結以上が上下のどちらかあるいは両方繋がっているやつ 
			tmpRegH = _mm_and_si128(linkTwo, _mm_slli_si128(linkTwo, 2));
			tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			//			↑orは少なくとも片方は繋がってる
			//	シードは			3連結以上	or	2連結以上しているツモが隣り合っている場所
			sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));



			//	シードに隣接する連結1も消える  _mm_xor_si128(sheedV, sheedH)は連結1　（シードを上下左右にずらして連結１の位置とandをとる）
			//	tmpRegV : シード + シードの上下に連結している連結1のツモの位置		↓シードを上にずらす													↓シードを下にずらす
			//	tmpRegH : シード + シードの左右に連結している連結1のツモの位置		↓シードを左にずらす													↓シードを右にずらす
			tmpRegV = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_epi16(sheed, 1)), _mm_and_si128(linkOne, _mm_srli_epi16(sheed, 1)));
			tmpRegH = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_si128(sheed, 2)), _mm_and_si128(linkOne, _mm_srli_si128(sheed, 2)));
			sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));
			vanish = _mm_or_si128(sheed, vanish); //4色見て最終的には消える位置全て１が立っている
		}
		//	vanishが0なら抜ける
		if ((vanishPtr[0] | vanishPtr[1]) == 0) break;
		//	ここを通る数が連鎖数
		count++;

		// ここで上下左右におじゃまがあったらフラグ立てればいい？(とりあえずお邪魔はなし)
		//tmpRegV = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_epi16(vanish, 1)), _mm_and_si128(ojama, _mm_srli_epi16(vanish, 1)));
		//tmpRegH = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_si128(vanish, 2)), _mm_and_si128(ojama, _mm_srli_si128(vanish, 2)));
		//vanish = _mm_or_si128(vanish, _mm_or_si128(tmpRegV, tmpRegH));

		//		PEXT, PDEPで連鎖後のフィールドを取得するイメージ
		//		PEXT => フィールド上の消えないツモを順番につめこむ
		//		フィールドは128ビット (勝手に8列 * 16行だと見立てている)　で定義している
		//		消えない位置(vanishのbit反転 ※空の場所も消えない判定でいい)を表しているマスクを与えれば 128 - 消えるツモ数 分のbitが後ろから順にセットされている結果を得られる
		//		
		//		PDEP => PEXT結果をフィールドに再配置する
		//		再配置用のマスクを与えればいい
		//		あらかじめ消えた数に応じて列ごと（16bit毎）の再配置位置を決定しておく　←　フィールドの下から1を立てその列の消えた数だけ一番上を0にするだけ
		// 
		//		例 フィールド(3列 * 5行)だとする
		//		0 0 0	← 1が4つなので消えるとすると	フィールド		11100 21220 32300
		//		0 2 0									消える位置		11100 01000 00000	
		//		1 2 3									消えない位置	00011 10111 11111
		//		1 1 2									PEXT結果			0 00222 00323  ←本来は左側０埋め
		//		1 2 3									消えた数			3	  1		0
		//												マスク			11000 11110 11111  ←フィールドの上側は消えた数分０にする
		//												PDEP			00    222   32300  ←本来は空いた場所は０埋め(PEXT1の下位ビットからマスクの1が立っている位置に下位から順に再配置される)
		//　　　こういうイメージで行う
		

		// 	pdepで再配置する際のマスクを設定する
		//	まず列ごと（16 * 8列）の消える数をvanishを分割してカウントすることで取得する
		//	その後maskに定義されている[1個消えるなら　→ 0111111111111000]というような値をpdepMaskにセット
		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		//  pext用にvanishのbit反転を用意する
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
		field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		//	ここで13段目復活
		//	残ったツモの上の空間をvanishにして消す(13段目にあればおちてくる)
		field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
		field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
		field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
		vanish = _mm_and_si128(_mm_or_si128(vanishOther, vanish13), _mm_xor_si128(_mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1])), _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff)));
		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));

		field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		//クリアする
		vanish = _mm_setzero_si128();
	}

	//13段目を元に戻す
	//その後フィールドのor(ツモ存在位置)を更新
	field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
	field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
	field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
	field_image[0] = _mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1]));

	return count;
}

//	保有連鎖数を取得 最初に１連鎖だけ色を指定して試す (速度のためtryChainに含まない)
int Thinking::specifiedChain(int color) {
	__m128i colorReg = _mm_setzero_si128();
	__m128i tmpRegV;
	__m128i tmpRegH;
	__m128i sheedV;
	__m128i sheedH;
	__m128i linkOne;
	__m128i linkTwo;
	__m128i sheed;
	__m128i vanish = _mm_setzero_si128();
	__m128i pdepMask;
	__m128i pextMask;
	__m128i thirteen[3];

	uint16_t* cnt = (uint16_t*)&vanish;
	uint64_t* pdepMaskPtr = (uint64_t*)&pdepMask;
	uint64_t* pextMaskPtr = (uint64_t*)&pextMask;
	uint64_t* fieldPtr1 = (uint64_t*)&field_image[1];
	uint64_t* fieldPtr2 = (uint64_t*)&field_image[2];
	uint64_t* fieldPtr3 = (uint64_t*)&field_image[3];
	uint64_t* vanishPtr = (uint64_t*)&vanish;
	//連鎖数
	int count = 0;

	//13段目の情報を保存
	//その後、フィールド(1～12段目)以外の情報を消す
	thirteen[0] = _mm_and_si128(vanish13, field_image[1]);
	thirteen[1] = _mm_and_si128(vanish13, field_image[2]);
	thirteen[2] = _mm_and_si128(vanish13, field_image[3]);
	field_image[1] = _mm_and_si128(vanishOther, field_image[1]);
	field_image[2] = _mm_and_si128(vanishOther, field_image[2]);
	field_image[3] = _mm_and_si128(vanishOther, field_image[3]);
	//	色を指定して発火を1度だけ試す
	switch (color) {
	case 1: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[2], field_image[1])); break;//赤
	case 2: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[1], field_image[2])); break;//緑
	case 3: colorReg = _mm_andnot_si128(field_image[3], _mm_and_si128(field_image[1], field_image[2])); break;//青
	case 4: colorReg = _mm_andnot_si128(field_image[2], _mm_andnot_si128(field_image[1], field_image[3])); break;//黄色
	//case 5: colorReg = _mm_andnot_si128(field_image[2], _mm_and_si128(field_image[1], field_image[3])); break; //紫
	}

	//1連結のツモを探す
	//  上下どちらかに同じ色があるツモが存在する位置にビットを立てる
	//	tmpRegV : 1つ上にずらしたフィールドとand　-> 1なら下にツモがあることになる
	//	sheedV	: tmpRegVのフィールドを下にずらしてxor -> 下のやつも1を立てる (左右にも繋がっているのは0に戻る)
	tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
	sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	//			↑xorは左右片方だけ繋がっている					
	//	左右どちらかに同じ色があるツモが存在する位置にビットを立てる
	//	tmpRegH : 1つ左にずらしたフィールドとand -> 1なら右にツモがあることになる
	//	sheedH	: tmpRegHのフィールドを左にずらしてxor -> 右のやつも1を立てる (左右にも繋がっているのは0に戻る)
	tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
	sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	//　連結1のツモ位置にビットを立てる
	//	左右どちらか　か　上下どちらか　だけビットを立てる
	linkOne = _mm_xor_si128(sheedV, sheedH);


	//  2連結のツモを探す
	//	2つ同じ色が連結しているツモが存在する位置にビットを立てる　（3つは除く） 条件は↓の3つ
	//						縦横両方繋がっている			or			横が繋がらず上下両方に繋がっているやつ										or					縦が繋がらず左右両方に繋がっているやつ				
	linkTwo = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
	//																								↑andは両方つながってる														↑andは両方つながってる


	//  3連結以上のツモを探す
	//	3つ以上同じ色が連結しているツモが存在する位置にビットを立てる																			
	//						4連結の条件→				（上下繋がっている　　　　　　　　　かつ　　　　　　　左右繋がっている　						or	→3連結の条件　(上下両方が繋がってる and   左右どちらかに繋がってる)				or		(左右両方に繋がっている   and  上下どちらかに繋がっている)															
	sheed = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128(_mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
	//											↑andは両方つながってる								↑andは両方つながってる																	↑andは両方つながってる														↑andは両方つながってる



	//	シード（消えるきっかけ）の位置にビットを立てる
	//	条件 = 連結3以上の奴 or 隣接の連結が2以上の連結2の奴
	//	
	//	連結2以上のやつ
	linkTwo = _mm_or_si128(sheed, linkTwo);
	//										   
	//	tmpRegV : 2連結以上が左右のどちらかあるいは両方繋がっているやつ
	tmpRegV = _mm_and_si128(linkTwo, _mm_slli_epi16(linkTwo, 1));
	tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	//			↑orは少なくとも片方は繋がってる
	//	tmpRegH : 2連結以上が上下のどちらかあるいは両方繋がっているやつ 
	tmpRegH = _mm_and_si128(linkTwo, _mm_slli_si128(linkTwo, 2));
	tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	//			↑orは少なくとも片方は繋がってる
	//	シードは			3連結以上	or	2連結以上しているツモが隣り合っている場所
	sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));



	//	シードに隣接する連結1も消える  _mm_xor_si128(sheedV, sheedH)は連結1　（シードを上下左右にずらして連結１の位置とandをとる）
	//	tmpRegV : シード + シードの上下に連結している連結1のツモの位置		↓シードを上にずらす													↓シードを下にずらす
	//	tmpRegH : シード + シードの左右に連結している連結1のツモの位置		↓シードを左にずらす													↓シードを右にずらす
	tmpRegV = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_epi16(sheed, 1)), _mm_and_si128(linkOne, _mm_srli_epi16(sheed, 1)));
	tmpRegH = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_si128(sheed, 2)), _mm_and_si128(linkOne, _mm_srli_si128(sheed, 2)));
	sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));
	vanish = _mm_or_si128(sheed, vanish); //4色見て最終的には消える位置全て１が立っている

	//消すものがなければ0を返す
	if ((vanishPtr[0] | vanishPtr[1]) == 0) {
		//13段目復活
		field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
		field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
		field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
		return 0;
	}
	count++;

	//発火する
	pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
	pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
	field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

	//13段目復活
	field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
	field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
	field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
	vanish = _mm_and_si128(_mm_or_si128(vanishOther, vanish13), _mm_xor_si128(_mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1])), _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff)));
	pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
	pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
	field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

	count += tryChain();
	return count;
}

//	評価値を計算する
int Thinking::evalScore(int chainMax_ready, int fireHeight) {

	int evalVal; //戻り値、評価値
	int heightAve; //高さの平均
	int heightValue; //高さのバランス評価値格納

	heightAve = (fieldHeight_image[0] + fieldHeight_image[1] + fieldHeight_image[2] + fieldHeight_image[3] + fieldHeight_image[4] + fieldHeight_image[5]) / 6;
	heightValue = (heightAve + 2 - fieldHeight_image[0]) * (heightAve + 2 - fieldHeight_image[0]);
	heightValue += (heightAve - fieldHeight_image[1]) * (heightAve - fieldHeight_image[1]);
	heightValue += (heightAve - 1 - fieldHeight_image[2]) * (heightAve - 1 - fieldHeight_image[2]);
	heightValue += (heightAve - 1 - fieldHeight_image[3]) * (heightAve - 1 - fieldHeight_image[3]);
	heightValue += (heightAve - fieldHeight_image[4]) * (heightAve - fieldHeight_image[4]);
	heightValue += (heightAve + 2 - fieldHeight_image[5]) * (heightAve + 2 - fieldHeight_image[5]);

	//評価値実計算部分（重みは適当）
	evalVal = -heightValue * 10;
	evalVal += chainMax_ready * (1000 * (heightAve + 1) / 10) + (100 * fireHeight);
	return evalVal;
}



//実行部分
void Thinking::thinking(int deep) {

	//	条件
	//	fiele_real(実際のフィールド情報)はセット済み
	//	nextTsumo（実際のフィールドの次のツモ、その次のツモ）はセット済み
	//  fieldHeigh_real（実際のフィールドの高さ情報）はセット済み

	//	変数設定
	//vector関係
	std::vector<value> vals;
	vals.reserve(10000);
	value val;

	int turn = 0; //image上で何ターン目かを格納
	int currentTsumo[2];  //このターンに落とすツモを格納
	int chain;
	int chainMax = 0; //戻り値みたいなもの
	int chainMax_ready;
	int first = 0;  //戻り値みたいなもの
	int fireHeight; //発火点の高さ - 評価用 

	int size;	//valsのサイズを保存

	//初期設定

	isDie = false;

	//***************開始******************
	//	
	//  ツモをランダムに作成して１手目と２手目は実際のに合わせる
	//	その後このターンに落とすツモを格納
	makeTsumo();
	currentTsumo[0] = tsumo[0];
	currentTsumo[1] = tsumo[1];
	// 
	//  ツモを実際に落とすことができる場所のみにstartとendを設定する
	//	フィールドの高さに応じて１手置く
	int start;
	int end;
	getFallPosition(start, end, fieldHeight_real);

	//	ツモを降らすのに使うため、高さをコピーしておく
	memcpy(fieldHeight_image, fieldHeight_real, sizeof(fieldHeight_real));


	//	start, endに応じてツモを１つ落とすを繰り返す
	// １手落とす
	//	消えるなら連鎖数を確認
	for (; start <= end; start++) {
		memcpy(field_image, field_real, sizeof(field_real));

		fallTsumo(start, currentTsumo);
		chain = tryChain();


		//	まず22手valsに格納する
		//	12段目にツモがあったら死んでいるのでスキップ
		if (_mm_testz_si128(field_image[0], die) == 1) {//_mm_testz_si128 => 全て0の場合が1
			memcpy(val.reg, field_image, sizeof(field_image));
			val.first = start;
			vals.emplace_back(val);

			//最大連鎖情報を更新
			if (chainMax < chain)
			{
				chainMax = chain;
				first = start;
			}
		}
		else {
			continue;
		}

	}

	//ここでvalsがない場合は置くところがない　=>　積みなので終わる
	if (size = vals.size() == 0) {
		isDie = true;
		return;
	}

	//	ここからさらに１手進めるを繰り返す(valsは*22に増えるのでRANGE幅で抑えるビームサーチ探索を行う)
	//	引数のdeep回数分探索する
	turn++; //１手はすでに進んだので2手目から
	for (; turn < deep; turn++) {

		//	valsのサイズを取得
		size = vals.size();

		//	カレントツモを更新
		currentTsumo[0] = tsumo[(turn * 2) % 128];
		currentTsumo[1] = tsumo[(turn * 2 + 1) % 128];

		//	valsを全走査してそれら全てを１手進める (最大RANGE + 22手見ることになる)
		for (int range = 0; range < size; ++range) {

			//	手を置く前のフィールド情報をコピーに持たせる
			memcpy(field_cp, vals[range].reg, sizeof(vals[range].reg));


			//	高さをコピーに持たせる
			setHeight(field_cp[0], fieldHeight_cp);


			//	start, endを取得
			getFallPosition(start, end, fieldHeight_cp);


			//	最大２２パターン置く
			//	連鎖があれば発火する
			for (; start <= end; ++start) { //va;ue.regから復元？
				//	評価ようの構造体に値を保存
				val.first = vals[range].first; //初手

				//	手を置く前のフィールドをセットする
				//	高さもセットする
				memcpy(field_image, field_cp, sizeof(field_cp));
				memcpy(fieldHeight_image, fieldHeight_cp, sizeof(fieldHeight_cp));


				//	一手置いて発火出来たらする
				//	最大連鎖数の情報を保持
				//	valに１手おいた後のフィールド情報をもたせる
				fallTsumo(start, currentTsumo);

				chain = tryChain();
				if (chainMax < chain) {
					chainMax = chain;
					first = vals[range].first;
				}
				memcpy(val.reg, field_image, sizeof(field_image));


				//	発火後の高さを取得
				//	４色のツモを最大２つ縦に６列すべてに置いて発火をためす
				//		＝＞何個置くかをst, ed決定する
				setHeight(field_image[0], fieldHeight_image);

				//	死んでいるものは試さない
				if (fieldHeight_image[2] > 11) {
					continue;
				}
				int st;
				int ed;
				getFallPosition(st, ed, fieldHeight_image);
				st = (st + 1) / 4; ed = ed / 4; //pattern => 列に変換(0～5)

				//	保有連鎖数を初期化
				chainMax_ready = 0;
				fireHeight = 0;
				__m128i fallPoint = Thinking::fallPoint; //(1, 1)を初期にする これを１れつずつずらす
				int fallCount;

				//	コピーにフィールド情報を持たせておく(毎回復元する)
				memcpy(field_cp2, field_image, sizeof(field_image));

				//	一列ずつループでツモを落としていく
				for (int row = st; row <= ed; ++row) {
					if (fieldHeight_image[row] > 11) { //12個以上なら
						fallCount = 0;
					}
					else if (fieldHeight_image[row] > 10) { //11個なら
						fallCount = 1;
					}
					else {  //10個以下なら
						fallCount = 2;
					}

					//	色を一色ずつ落としていく
					for (int color = 1; color <= 4; ++color) {
						//	落とす数だけループする
						for (int cnt = 0; cnt < fallCount; ++cnt) {
							//fallCountの数だけ落とす
							field_image[color & 0x1] = _mm_or_si128(field_image[color & 0x1], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));
							field_image[((color >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((color >> 1) & 0x1) * 2], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));
							field_image[((color >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((color >> 2) & 0x1) * 3], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));

							//保有連鎖数情報を格納
							chain = specifiedChain(color);
							if (chain > 0) {
								//聴牌している連鎖数を保持
								if (chain > chainMax_ready) {
									fireHeight = fieldHeight_image[row];
									chainMax_ready = chain;

								}

								//1個目で消えたら２個目は見なくていいので
								break;
							}

						}
						//フィールド情報を復元する
						memcpy(field_image, field_cp2, sizeof(field_cp2));
					}
					fallPoint = _mm_srli_si128(fallPoint, 2); //次の列の一番下(1 + row, 1)にビットを移動

				}

				//****************評価値を計算(発火可能な連鎖数 + 窪みの少なさ)**************************
				val.score = evalScore(chainMax_ready, fireHeight);

				//評価値を保存して配列に追加
				vals.emplace_back(val);

			}

		}

		//ここで上位size手を削除
		vals.erase(vals.begin(), vals.begin() + size);
		//上位(RANGE)手を保存
		std::partial_sort(vals.begin(), vals.begin() + (vals.size() < RANGE ? vals.size() : RANGE), vals.end(), std::greater<value>());
		vals.erase(vals.begin() + (vals.size() < RANGE ? vals.size() : RANGE), vals.end());
	}
	pat[first] += chainMax;
	return;
}


//*******************テスト用*****************

//	reg[]からフィールドを確認
void Thinking::fieldCheck(__m128i reg[]) {
	int field[8][16];

	uint64_t* llA, * llB, * llC;
	uint64_t A[2], B[2], C[2];
	llA = (uint64_t*)&reg[1];
	llB = (uint64_t*)&reg[2];
	llC = (uint64_t*)&reg[3];

	A[0] = llA[0], A[1] = llA[1];
	B[0] = llB[0], B[1] = llB[1];
	C[0] = llC[0], C[1] = llC[1];
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			field[i][j] = 0;
		}
	}
	for (int i = 7; i >= 4; i--) {
		for (int j = 15; j >= 0; j--) {
			field[i][j] ^= (0b1 & C[0]);
			(field[i][j] <<= 1) ^= (0b1 & B[0]);
			(field[i][j] <<= 1) ^= (0b1 & A[0]);
			A[0] >>= 1;
			B[0] >>= 1;
			C[0] >>= 1;
		}
	}
	for (int i = 3; i >= 0; i--) {
		for (int j = 15; j >= 0; j--) {
			field[i][j] ^= (0b1 & C[1]);
			(field[i][j] <<= 1) ^= (0b1 & B[1]);
			(field[i][j] <<= 1) ^= (0b1 & A[1]);
			A[1] >>= 1;
			B[1] >>= 1;
			C[1] >>= 1;
		}
	}

	std::cout << "チェック" << std::endl;

}