#pragma once
#include <immintrin.h>
#include <intrin.h>
#include <vector>
#include <string>
//#define RANGE = 500; 

// �]���p�̍\���̒�`
struct value { //�]���p
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
	//�e�X�g�p
	void fieldCheck(__m128i reg[]);

	//�R���X�g���N�^
	Thinking() {
		//tsumo��128�m��
		std::vector<int> temp(128);
		tsumo.swap(temp);

		//�]���p�ɏd�݂�ݒ�i�ǂ������ɂ��Ă������j  //������͂��Ղ葤�ł����肽��
		fireHeight_W = 100;  //	���Βn�_�̍����̏d��
		chain_W = 1000;  //			�A�����̏d��
		field_W = 10; //			�`�̏d��

	};

	//���s�������
	void thinking(int deep);
	static __m128i die;
	static bool isDie;
	static int RANGE; //

	//	�t�B�[���h�̒l�����W�X�^�Ɏ��� //0: �S�Ă�or 1��1bit�� 2��2bt�� 3��3bit��
	//	�F�͂V�F�܂ň��� => 000�`111
	//	0�͋�,1, 2, 3, 4���Ղ��4�F��\��
	static __m128i field_real[4];
	__m128i field_image[4];
	__m128i field_cp[4] = {};
	__m128i field_cp2[4] = {};
	//���݂̃t�B�[���h(�`��Ɏg�p���Ă���z��)���Z�b�g����
	static void setField(int nowField[8][16]);



	//***********�c���쐬*******************
	std::vector<int> tsumo;
	static int nextTsumo[6];
	void makeTsumo();


	//******�t�B�[���h�̍���*******:
	//	�t�B�[���h�̍����i�[�p
	static int fieldHeight_real[6];
	int fieldHeight_image[6]; //�v�Z�Łi�����Z�Ə�]�ł�����j
	int fieldHeight_cp[6];

	//	�t�B�[���h�̍�����z��Ŏ���
	static void setHeight(__m128i reg, int array[]);


	//�c����������ʒu���擾
	void getFallPosition(int &start, int &last, int height[]);
	
	
	//************���̎��u��**********
	const __m128i fallPoint = _mm_set_epi64x(0x0000400000000000, 0x0); //(1, 1)�̃r�b�g�������Ă�
	const __m128i vanish14 = _mm_set_epi64x(0x0000000200020002, 0x0002000200020000); //14�i�ڂɃr�b�g�������Ă���
	const __m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13�i�ڂɃr�b�g�������Ă���
	const __m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);
	// 7ff8
	// �t�B�[���h14�}�X�@14�A13�i�ڈȊO�Ƀr�b�g�������Ă�����
	// |--------------|
	//0 11111111111100 0

	
	//�����͉��p�^�[���ڂ��ƁA���Ƃ��c��
	void fallTsumo(int pat, int tsumo[]);



	//************�A��������************
	//�A�������p�̃}�X�N
	const unsigned int mask[14] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000};
	//�A�����Ă��邩������
	int tryChain();

	//�w��E���� - �ۗL�A�������擾
	int specifiedChain(int color);



	//*************�]��*************
	//	�]���l�p
	int fireHeight_W;  //	���Βn�_�̍����̏d��
	int chain_W;  //			�A�����̏d��
	int field_W;  //			�`�̏d��

	//	�]���l�v�Z
	int evalScore(int chainMax_ready, int fireHeight);


	//*************���薈�ɘA�������i�[**********
	static int pat[22];
	static void resetPat();
};

//static������
__declspec(selectany) __m128i Thinking::die = (_mm_set_epi64x(0x8, 0x0));  //3���12�i�ڂ̃r�b�g�������Ă���
__declspec(selectany) bool Thinking::isDie = false;
__declspec(selectany) __m128i Thinking::field_real[] = {};
__declspec(selectany) int Thinking::nextTsumo[] = {};
__declspec(selectany) int Thinking::fieldHeight_real[] = {};
__declspec(selectany) int Thinking::pat[] = {};
