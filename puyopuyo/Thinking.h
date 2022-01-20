#pragma once
#include <immintrin.h>
#include <intrin.h>
#include <vector>
#include <string>
//#define RANGE = 500; 

// 評価用の構造体定義
struct value { //評価用
	value() {}
	int score = -10000;
	//int range = 0;
	int first = 0;
	int chain = 0;
	//int cnt = 0;]
	__m128i reg[4] = {};

	//unsigned long long fi[6] = {};
	bool operator>(const value& right) const {
		//return cnt == right.cnt ? value > right.value : cnt > right.cnt;
		return score > right.score;
	}

};

class Thinking
{
public:
	//テスト用
	void fieldCheck(__m128i reg[]);

	//コンストラクタ
	Thinking() {
		//tsumoを128個確保
		std::vector<int> temp(128);
		tsumo.swap(temp);

	};

	//実行するもの
	void thinking(int deep);
	static __m128i die;
	static bool isDie;
	static int RANGE; //

	//	フィールドの値をレジスタに持つ //0: 全てのor 1が1bit目 2が2bt目 3が3bit目
	//	色は７色まで扱う => 000～111
	//	0は空白,1, 2, 3, 4がぷよの4色を表す
	static __m128i field_real[4];
	__m128i field_image[4];
	__m128i field_cp[4] = {};
	__m128i field_cp2[4] = {};
	//現在のフィールド(描画に使用している配列)をセットする
	static void setField(int nowField[8][16]);



	//***********ツモ作成*******************
	std::vector<int> tsumo;
	static int nextTsumo[6];
	void makeTsumo();


	//******フィールドの高さ*******:
	//	フィールドの高さ格納用
	static int fieldHeight_real[6];
	int fieldHeight_image[6]; //計算で（足し算と剰余でいける）
	int fieldHeight_cp[6];

	//	フィールドの高さを配列で持つ
	static void setHeight(__m128i reg, int array[]);


	//ツモをおける位置を取得
	void getFallPosition(int &start, int &last, int height[]);
	
	
	//************次の手を置く**********
	const __m128i fallPoint = _mm_set_epi64x(0x0000400000000000, 0x0); //(1, 1)のビットが立ってる
	const __m128i vanish14 = _mm_set_epi64x(0x0000000200020002, 0x0002000200020000); //14段目にビットが立っている
	const __m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13段目にビットが立っている
	const __m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);
	// 7ff8
	// フィールド14マス
	// |--------------|
	//0 11111111111100 0

	
	//引数は何パターン目かと、落とすツモ
	void fallTsumo(int pat, int tsumo[]);



	//************連鎖を試す************
	//連鎖処理用のマスク
	const unsigned int mask[14] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000};
	//連鎖しているかを試す
	int tryChain();

	//指定職発火 - 保有連鎖数を取得
	int specifiedChain(int color);



	//*************評価*************
	//	評価値用
	int chainHeight_W = 0;  //	発火地点の高さの重み
	int chain_W = 0;  //			連鎖数の重み
	int field_W = 0;  //			形の重み

	//	評価値計算
	int evalScore(int chainMax_ready, int fireHeight);


	//*************初手毎に連鎖数を格納**********
	static int pat[22];
	static void resetPat();
};

//static初期化
__declspec(selectany) __m128i Thinking::die = (_mm_set_epi64x(0x8, 0x0));  //3列目12段目のビットが立っている
__declspec(selectany) bool Thinking::isDie = false;
__declspec(selectany) __m128i Thinking::field_real[] = {};
__declspec(selectany) int Thinking::nextTsumo[] = {};
__declspec(selectany) int Thinking::fieldHeight_real[] = {};
__declspec(selectany) int Thinking::pat[] = {};
