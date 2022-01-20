#include "Thinking.h"
#include <random>
#include <vector>
#include <iostream>
#include <immintrin.h>
#include <string>

int Thinking::RANGE = 400;

//pat���Z�b�g
void Thinking::resetPat() {
	for (int i = 0; i < 22; i++) {
		pat[i] = 0;
	}
}

//�z�񂩂�field�ɃZ�b�g
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


//�c���������_���ɍ쐬(�S�F128��ϓ�)
void Thinking::makeTsumo() {
	for (int i = 0; i < 128; i++) {
		tsumo[i] = i / 32 + 1;
	}
	std::random_device get_rand_dev;
	std::mt19937 get_rand_mt(get_rand_dev()); // seed�ɗ������w��
	std::shuffle(tsumo.begin(), tsumo.end(), get_rand_mt);
	//����Ƃ��̎��͌�����c���ɍ��킹��(���[�v���͑����H)
	tsumo[0] = nextTsumo[0];
	tsumo[1] = nextTsumo[1];
	tsumo[2] = nextTsumo[2];
	tsumo[3] = nextTsumo[3];
	tsumo[4] = nextTsumo[4];
	tsumo[5] = nextTsumo[5];
}

//	������z��Ɋi�[
void Thinking::setHeight(__m128i reg, int array[]) {
	uint16_t* val = (uint16_t*)&reg;
	for (int i = 0; i < 6; i++) {
		array[i] = __popcnt16(val[6 - i]);
	}
	return;
}

//�c����������ʒu���擾(�����ɃZ�b�g���ĕԂ�)
void Thinking::getFallPosition(int& start, int& end, int height[]) {
	//13�i�̕ǂ͉z�����Ȃ�
	if (height[1] < 13) {
		if (height[0] < 13) {
			start = 0;
		}
		else {
			start = 3; //��̃c����14�i�ڂɂ̂�����
		}

	}
	else {
		start = 7; //��̃c����14�i�ڂɂ̂�����
	}

	if (height[3] < 13) {
		if (height[4] < 13) {
			if (height[5] < 13) {
				end = 21;
			}
			else {
				end = 18; //��̃c����14�i�ڂɂ̂�����
			}
		}
		else {
			end = 14; //��̃c����14�i�ڂɂ̂�����
		}
	}
	else {
		end = 10; //��̃c����14�i�ڂɂ̂�����
	}
	return;
}

//	���̎��u��
void Thinking::fallTsumo(int pat, int tsumo[]) {
	//	fallpoint <- (1, 1)�̈ʒu
	//	fallpoint1 <- �c�u�����̏㑤�Ɖ��u�����̍����@�O�`�R�͂P��ڂS�`�V�͂Q���
	//	fallpoint2 <- �c�u�����̉����Ɖ��u�����̉E���@�Q�`�T�͂Q��ڂU�`�P�O�͂R���
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
	fallPoint1 = _mm_andnot_si128(vanish14, _mm_srli_epi16(fallPoint1, fieldHeight_image[pat / 4] + (((pat + 2) / 2) % 2))); //�@(((pat + 2) / 2) % 2)) <= ��̂Ƃ��Ɖ��̂Ƃ�������̂�
	fallPoint2 = _mm_andnot_si128(vanish14, _mm_srli_epi16(fallPoint2, fieldHeight_image[(pat + 2) / 4]));
	//��14�i�ڂ͍폜

	//1�p�^�[�����u��

	//�@���̂܂ܗ������̏オ�Z�����~�Ƃ����
	//�@�@�O�@�P�@�Q�@�@�R
	//�@�@�~�@�Z�@
	//�@�@�Z�@�~�@�~�Z�@�Z�~�@�ɂ�����
	//�@�Ƃ������łQ�Q��u��
	//	fallpoint1, 2�ɑΉ�����Z�Ɓ~��1�育�Ƃɓ��ꂩ��邱�ƂɂȂ�


	//	�c���̐F�ɉ�����fallPoint�𔽉f����
	//	field_image��field_cp�Ɠ����ɂȂ��Ă���O��

	//	1�߂�u��
	field_image[tsumo[(pat + 1) % 2] & 0x1] = _mm_or_si128(field_image[tsumo[(pat + 1) % 2] & 0x1], fallPoint1);
	field_image[((tsumo[(pat + 1) % 2] >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((tsumo[(pat + 1) % 2] >> 1) & 0x1) * 2], fallPoint1);
	field_image[((tsumo[(pat + 1) % 2] >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((tsumo[(pat + 1) % 2] >> 2) & 0x1) * 3], fallPoint1);

	//	2�߂�u��
	field_image[tsumo[pat % 2] & 0x1] = _mm_or_si128(field_image[tsumo[pat % 2] & 0x1], fallPoint2);
	field_image[((tsumo[pat % 2] >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((tsumo[pat % 2] >> 1) & 0x1) * 2], fallPoint2);
	field_image[((tsumo[pat % 2] >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((tsumo[pat % 2] >> 2) & 0x1) * 3], fallPoint2);

	//�R�t�B�[���h��or(�c�����݈ʒu)���X�V
	field_image[0] = _mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1]));

	return;

}

//	�A�����Ă��邩������
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
	//__m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13�i�ڂɃr�b�g�������Ă���
	//__m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);
	
	uint16_t* cnt = (uint16_t*)&vanish;
	uint64_t* pdepMaskPtr = (uint64_t*)&pdepMask;
	uint64_t* pextMaskPtr = (uint64_t*)&pextMask;
	uint64_t* fieldPtr1 = (uint64_t*)&field_image[1];
	uint64_t* fieldPtr2 = (uint64_t*)&field_image[2];
	uint64_t* fieldPtr3 = (uint64_t*)&field_image[3];
	uint64_t* vanishPtr = (uint64_t*)&vanish;

	//�A������Ԃ�
	int count = 0;

	while (true) {
		//13�i�ڂ̏���ۑ�
		//���̌�A�t�B�[���h(1�`12�i��)�ȊO�̏�������
		thirteen[0] = _mm_and_si128(vanish13, field_image[1]);
		thirteen[1] = _mm_and_si128(vanish13, field_image[2]);
		thirteen[2] = _mm_and_si128(vanish13, field_image[3]);
		field_image[1] = _mm_and_si128(vanishOther, field_image[1]);
		field_image[2] = _mm_and_si128(vanishOther, field_image[2]);
		field_image[3] = _mm_and_si128(vanishOther, field_image[3]);
		for (int color = 1; color < 5; color++) {
			//��F������
			switch (color) {																						     //	    reg3   reg2   reg1	
			case 1: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[2], field_image[1])); break;//�� :  0		0	   1
			case 2: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[1], field_image[2])); break;//�� :  0      1      0
			case 3: colorReg = _mm_andnot_si128(field_image[3], _mm_and_si128(field_image[1], field_image[2])); break;//�� :     0      1      1
			case 4: colorReg = _mm_andnot_si128(field_image[2], _mm_andnot_si128(field_image[1], field_image[3])); break;//���F  1      0      0
			//case 5: colorReg = _mm_andnot_si128(field_image[2], _mm_and_si128(field_image[1], field_image[3])); break; //��    1      0      1
			}

			//1�A���̃c����T��
			//  �㉺�ǂ��炩�ɓ����F������c�������݂���ʒu�Ƀr�b�g�𗧂Ă�
			//	tmpRegV : 1��ɂ��炵���t�B�[���h��and�@-> 1�Ȃ牺�Ƀc�������邱�ƂɂȂ�
			//	sheedV	: tmpRegV�̃t�B�[���h�����ɂ��炵��xor -> ���̂��1�𗧂Ă� (���E�ɂ��q�����Ă���̂�0�ɖ߂�)
			tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
			sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			//			��xor�͍��E�Е������q�����Ă���					
			//	���E�ǂ��炩�ɓ����F������c�������݂���ʒu�Ƀr�b�g�𗧂Ă�
			//	tmpRegH : 1���ɂ��炵���t�B�[���h��and -> 1�Ȃ�E�Ƀc�������邱�ƂɂȂ�
			//	sheedH	: tmpRegH�̃t�B�[���h�����ɂ��炵��xor -> �E�̂��1�𗧂Ă� (���E�ɂ��q�����Ă���̂�0�ɖ߂�)
			tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
			sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			//�@�A��1�̃c���ʒu�Ƀr�b�g�𗧂Ă�
			//	���E�ǂ��炩�@���@�㉺�ǂ��炩�@�����r�b�g�𗧂Ă�
			linkOne = _mm_xor_si128(sheedV, sheedH);


			//  2�A���̃c����T��
			//	2�����F���A�����Ă���c�������݂���ʒu�Ƀr�b�g�𗧂Ă�@�i3�͏����j �����́���3��
			//						�c�������q�����Ă���			or			�����q���炸�㉺�����Ɍq�����Ă�����										or					�c���q���炸���E�����Ɍq�����Ă�����				
			linkTwo = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
			//																								��and�͗����Ȃ����Ă�														��and�͗����Ȃ����Ă�


			//  3�A���ȏ�̃c����T��
			//	3�ȏ㓯���F���A�����Ă���c�������݂���ʒu�Ƀr�b�g�𗧂Ă�																			
			//						4�A���̏�����				�i�㉺�q�����Ă���@�@�@�@�@�@�@�@�@���@�@�@�@�@�@�@���E�q�����Ă���@						or	��3�A���̏����@(�㉺�������q�����Ă� and   ���E�ǂ��炩�Ɍq�����Ă�)				or		(���E�����Ɍq�����Ă���   and  �㉺�ǂ��炩�Ɍq�����Ă���)															
			sheed = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128( _mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))) );
			//											��and�͗����Ȃ����Ă�								��and�͗����Ȃ����Ă�																	��and�͗����Ȃ����Ă�														��and�͗����Ȃ����Ă�
			 
			

			//	�V�[�h�i�����邫�������j�̈ʒu�Ƀr�b�g�𗧂Ă�
			//	���� = �A��3�ȏ�̓z or �אڂ̘A����2�ȏ�̘A��2�̓z
			//	
			//	�A��2�ȏ�̂��
			linkTwo = _mm_or_si128(sheed, linkTwo);
			//										   
			//	tmpRegV : 2�A���ȏオ���E�̂ǂ��炩���邢�͗����q�����Ă�����
			tmpRegV = _mm_and_si128(linkTwo, _mm_slli_epi16(linkTwo, 1));
			tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			//			��or�͏��Ȃ��Ƃ��Е��͌q�����Ă�
			//	tmpRegH : 2�A���ȏオ�㉺�̂ǂ��炩���邢�͗����q�����Ă����� 
			tmpRegH = _mm_and_si128(linkTwo, _mm_slli_si128(linkTwo, 2));
			tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			//			��or�͏��Ȃ��Ƃ��Е��͌q�����Ă�
			//	�V�[�h��			3�A���ȏ�	or	2�A���ȏサ�Ă���c�����ׂ荇���Ă���ꏊ
			sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));



			//	�V�[�h�ɗאڂ���A��1��������  _mm_xor_si128(sheedV, sheedH)�͘A��1�@�i�V�[�h���㉺���E�ɂ��炵�ĘA���P�̈ʒu��and���Ƃ�j
			//	tmpRegV : �V�[�h + �V�[�h�̏㉺�ɘA�����Ă���A��1�̃c���̈ʒu		���V�[�h����ɂ��炷													���V�[�h�����ɂ��炷
			//	tmpRegH : �V�[�h + �V�[�h�̍��E�ɘA�����Ă���A��1�̃c���̈ʒu		���V�[�h�����ɂ��炷													���V�[�h���E�ɂ��炷
			tmpRegV = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_epi16(sheed, 1)), _mm_and_si128(linkOne, _mm_srli_epi16(sheed, 1)));
			tmpRegH = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_si128(sheed, 2)), _mm_and_si128(linkOne, _mm_srli_si128(sheed, 2)));
			sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));
			vanish = _mm_or_si128(sheed, vanish); //4�F���čŏI�I�ɂ͏�����ʒu�S�ĂP�������Ă���
		}
		//	vanish��0�Ȃ甲����
		if ((vanishPtr[0] | vanishPtr[1]) == 0) break;
		//	������ʂ鐔���A����
		count++;

		// �����ŏ㉺���E�ɂ�����܂���������t���O���Ă�΂����H(�Ƃ肠�������ז��͂Ȃ�)
		//tmpRegV = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_epi16(vanish, 1)), _mm_and_si128(ojama, _mm_srli_epi16(vanish, 1)));
		//tmpRegH = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_si128(vanish, 2)), _mm_and_si128(ojama, _mm_srli_si128(vanish, 2)));
		//vanish = _mm_or_si128(vanish, _mm_or_si128(tmpRegV, tmpRegH));

		//		PEXT, PDEP�ŘA����̃t�B�[���h���擾����C���[�W
		//		PEXT => �t�B�[���h��̏����Ȃ��c�������Ԃɂ߂���
		//		�t�B�[���h��128�r�b�g (�����8�� * 16�s���ƌ����ĂĂ���)�@�Œ�`���Ă���
		//		�����Ȃ��ʒu(vanish��bit���] ����̏ꏊ�������Ȃ�����ł���)��\���Ă���}�X�N��^����� 128 - ������c���� ����bit����납�珇�ɃZ�b�g����Ă��錋�ʂ𓾂���
		//		
		//		PDEP => PEXT���ʂ��t�B�[���h�ɍĔz�u����
		//		�Ĕz�u�p�̃}�X�N��^����΂���
		//		���炩���ߏ��������ɉ����ė񂲂Ɓi16bit���j�̍Ĕz�u�ʒu�����肵�Ă����@���@�t�B�[���h�̉�����1�𗧂Ă��̗�̏�������������ԏ��0�ɂ��邾��
		// 
		//		�� �t�B�[���h(3�� * 5�s)���Ƃ���
		//		0 0 0	�� 1��4�Ȃ̂ŏ�����Ƃ����	�t�B�[���h		11100 21220 32300
		//		0 2 0									������ʒu		11100 01000 00000	
		//		1 2 3									�����Ȃ��ʒu	00011 10111 11111
		//		1 1 2									PEXT����			0 00222 00323  ���{���͍����O����
		//		1 2 3									��������			3	  1		0
		//												�}�X�N			11000 11110 11111  ���t�B�[���h�̏㑤�͏����������O�ɂ���
		//												PDEP			00    222   32300  ���{���͋󂢂��ꏊ�͂O����(PEXT1�̉��ʃr�b�g����}�X�N��1�������Ă���ʒu�ɉ��ʂ��珇�ɍĔz�u�����)
		//�@�@�@���������C���[�W�ōs��
		

		// 	pdep�ōĔz�u����ۂ̃}�X�N��ݒ肷��
		//	�܂��񂲂Ɓi16 * 8��j�̏����鐔��vanish�𕪊����ăJ�E���g���邱�ƂŎ擾����
		//	���̌�mask�ɒ�`����Ă���[1������Ȃ�@�� 0111111111111000]�Ƃ����悤�Ȓl��pdepMask�ɃZ�b�g
		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		//  pext�p��vanish��bit���]��p�ӂ���
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
		field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		//	������13�i�ڕ���
		//	�c�����c���̏�̋�Ԃ�vanish�ɂ��ď���(13�i�ڂɂ���΂����Ă���)
		field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
		field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
		field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
		vanish = _mm_and_si128(_mm_or_si128(vanishOther, vanish13), _mm_xor_si128(_mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1])), _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff)));
		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));

		field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		//�N���A����
		vanish = _mm_setzero_si128();
	}

	//13�i�ڂ����ɖ߂�
	//���̌�t�B�[���h��or(�c�����݈ʒu)���X�V
	field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
	field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
	field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
	field_image[0] = _mm_or_si128(field_image[3], _mm_or_si128(field_image[2], field_image[1]));

	return count;
}

//	�ۗL�A�������擾 �ŏ��ɂP�A�������F���w�肵�Ď��� (���x�̂���tryChain�Ɋ܂܂Ȃ�)
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
	//�A����
	int count = 0;

	//13�i�ڂ̏���ۑ�
	//���̌�A�t�B�[���h(1�`12�i��)�ȊO�̏�������
	thirteen[0] = _mm_and_si128(vanish13, field_image[1]);
	thirteen[1] = _mm_and_si128(vanish13, field_image[2]);
	thirteen[2] = _mm_and_si128(vanish13, field_image[3]);
	field_image[1] = _mm_and_si128(vanishOther, field_image[1]);
	field_image[2] = _mm_and_si128(vanishOther, field_image[2]);
	field_image[3] = _mm_and_si128(vanishOther, field_image[3]);
	//	�F���w�肵�Ĕ��΂�1�x��������
	switch (color) {
	case 1: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[2], field_image[1])); break;//��
	case 2: colorReg = _mm_andnot_si128(field_image[3], _mm_andnot_si128(field_image[1], field_image[2])); break;//��
	case 3: colorReg = _mm_andnot_si128(field_image[3], _mm_and_si128(field_image[1], field_image[2])); break;//��
	case 4: colorReg = _mm_andnot_si128(field_image[2], _mm_andnot_si128(field_image[1], field_image[3])); break;//���F
	//case 5: colorReg = _mm_andnot_si128(field_image[2], _mm_and_si128(field_image[1], field_image[3])); break; //��
	}

	//1�A���̃c����T��
	//  �㉺�ǂ��炩�ɓ����F������c�������݂���ʒu�Ƀr�b�g�𗧂Ă�
	//	tmpRegV : 1��ɂ��炵���t�B�[���h��and�@-> 1�Ȃ牺�Ƀc�������邱�ƂɂȂ�
	//	sheedV	: tmpRegV�̃t�B�[���h�����ɂ��炵��xor -> ���̂��1�𗧂Ă� (���E�ɂ��q�����Ă���̂�0�ɖ߂�)
	tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
	sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	//			��xor�͍��E�Е������q�����Ă���					
	//	���E�ǂ��炩�ɓ����F������c�������݂���ʒu�Ƀr�b�g�𗧂Ă�
	//	tmpRegH : 1���ɂ��炵���t�B�[���h��and -> 1�Ȃ�E�Ƀc�������邱�ƂɂȂ�
	//	sheedH	: tmpRegH�̃t�B�[���h�����ɂ��炵��xor -> �E�̂��1�𗧂Ă� (���E�ɂ��q�����Ă���̂�0�ɖ߂�)
	tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
	sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	//�@�A��1�̃c���ʒu�Ƀr�b�g�𗧂Ă�
	//	���E�ǂ��炩�@���@�㉺�ǂ��炩�@�����r�b�g�𗧂Ă�
	linkOne = _mm_xor_si128(sheedV, sheedH);


	//  2�A���̃c����T��
	//	2�����F���A�����Ă���c�������݂���ʒu�Ƀr�b�g�𗧂Ă�@�i3�͏����j �����́���3��
	//						�c�������q�����Ă���			or			�����q���炸�㉺�����Ɍq�����Ă�����										or					�c���q���炸���E�����Ɍq�����Ă�����				
	linkTwo = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
	//																								��and�͗����Ȃ����Ă�														��and�͗����Ȃ����Ă�


	//  3�A���ȏ�̃c����T��
	//	3�ȏ㓯���F���A�����Ă���c�������݂���ʒu�Ƀr�b�g�𗧂Ă�																			
	//						4�A���̏�����				�i�㉺�q�����Ă���@�@�@�@�@�@�@�@�@���@�@�@�@�@�@�@���E�q�����Ă���@						or	��3�A���̏����@(�㉺�������q�����Ă� and   ���E�ǂ��炩�Ɍq�����Ă�)				or		(���E�����Ɍq�����Ă���   and  �㉺�ǂ��炩�Ɍq�����Ă���)															
	sheed = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128(_mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));
	//											��and�͗����Ȃ����Ă�								��and�͗����Ȃ����Ă�																	��and�͗����Ȃ����Ă�														��and�͗����Ȃ����Ă�



	//	�V�[�h�i�����邫�������j�̈ʒu�Ƀr�b�g�𗧂Ă�
	//	���� = �A��3�ȏ�̓z or �אڂ̘A����2�ȏ�̘A��2�̓z
	//	
	//	�A��2�ȏ�̂��
	linkTwo = _mm_or_si128(sheed, linkTwo);
	//										   
	//	tmpRegV : 2�A���ȏオ���E�̂ǂ��炩���邢�͗����q�����Ă�����
	tmpRegV = _mm_and_si128(linkTwo, _mm_slli_epi16(linkTwo, 1));
	tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	//			��or�͏��Ȃ��Ƃ��Е��͌q�����Ă�
	//	tmpRegH : 2�A���ȏオ�㉺�̂ǂ��炩���邢�͗����q�����Ă����� 
	tmpRegH = _mm_and_si128(linkTwo, _mm_slli_si128(linkTwo, 2));
	tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	//			��or�͏��Ȃ��Ƃ��Е��͌q�����Ă�
	//	�V�[�h��			3�A���ȏ�	or	2�A���ȏサ�Ă���c�����ׂ荇���Ă���ꏊ
	sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));



	//	�V�[�h�ɗאڂ���A��1��������  _mm_xor_si128(sheedV, sheedH)�͘A��1�@�i�V�[�h���㉺���E�ɂ��炵�ĘA���P�̈ʒu��and���Ƃ�j
	//	tmpRegV : �V�[�h + �V�[�h�̏㉺�ɘA�����Ă���A��1�̃c���̈ʒu		���V�[�h����ɂ��炷													���V�[�h�����ɂ��炷
	//	tmpRegH : �V�[�h + �V�[�h�̍��E�ɘA�����Ă���A��1�̃c���̈ʒu		���V�[�h�����ɂ��炷													���V�[�h���E�ɂ��炷
	tmpRegV = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_epi16(sheed, 1)), _mm_and_si128(linkOne, _mm_srli_epi16(sheed, 1)));
	tmpRegH = _mm_or_si128(_mm_and_si128(linkOne, _mm_slli_si128(sheed, 2)), _mm_and_si128(linkOne, _mm_srli_si128(sheed, 2)));
	sheed = _mm_or_si128(sheed, _mm_or_si128(tmpRegV, tmpRegH));
	vanish = _mm_or_si128(sheed, vanish); //4�F���čŏI�I�ɂ͏�����ʒu�S�ĂP�������Ă���

	//�������̂��Ȃ����0��Ԃ�
	if ((vanishPtr[0] | vanishPtr[1]) == 0) {
		//13�i�ڕ���
		field_image[1] = _mm_or_si128(thirteen[0], field_image[1]);
		field_image[2] = _mm_or_si128(thirteen[1], field_image[2]);
		field_image[3] = _mm_or_si128(thirteen[2], field_image[3]);
		return 0;
	}
	count++;

	//���΂���
	pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
	pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
	field_image[1] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[2] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
	field_image[3] = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

	//13�i�ڕ���
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

//	�]���l���v�Z����
int Thinking::evalScore(int chainMax_ready, int fireHeight) {

	int evalVal; //�߂�l�A�]���l
	int heightAve; //�����̕���
	int heightValue; //�����̃o�����X�]���l�i�[

	heightAve = (fieldHeight_image[0] + fieldHeight_image[1] + fieldHeight_image[2] + fieldHeight_image[3] + fieldHeight_image[4] + fieldHeight_image[5]) / 6;
	heightValue = (heightAve + 2 - fieldHeight_image[0]) * (heightAve + 2 - fieldHeight_image[0]);
	heightValue += (heightAve - fieldHeight_image[1]) * (heightAve - fieldHeight_image[1]);
	heightValue += (heightAve - 1 - fieldHeight_image[2]) * (heightAve - 1 - fieldHeight_image[2]);
	heightValue += (heightAve - 1 - fieldHeight_image[3]) * (heightAve - 1 - fieldHeight_image[3]);
	heightValue += (heightAve - fieldHeight_image[4]) * (heightAve - fieldHeight_image[4]);
	heightValue += (heightAve + 2 - fieldHeight_image[5]) * (heightAve + 2 - fieldHeight_image[5]);

	//�]���l���v�Z�����i�d�݂͓K���j
	evalVal = -heightValue * 10;
	evalVal += chainMax_ready * (1000 * (heightAve + 1) / 10) + (100 * fireHeight);
	return evalVal;
}



//���s����
void Thinking::thinking(int deep) {

	//	����
	//	fiele_real(���ۂ̃t�B�[���h���)�̓Z�b�g�ς�
	//	nextTsumo�i���ۂ̃t�B�[���h�̎��̃c���A���̎��̃c���j�̓Z�b�g�ς�
	//  fieldHeigh_real�i���ۂ̃t�B�[���h�̍������j�̓Z�b�g�ς�

	//	�ϐ��ݒ�
	//vector�֌W
	std::vector<value> vals;
	vals.reserve(10000);
	value val;

	int turn = 0; //image��ŉ��^�[���ڂ����i�[
	int currentTsumo[2];  //���̃^�[���ɗ��Ƃ��c�����i�[
	int chain;
	int chainMax = 0; //�߂�l�݂����Ȃ���
	int chainMax_ready;
	int first = 0;  //�߂�l�݂����Ȃ���
	int fireHeight; //���Γ_�̍��� - �]���p 

	int size;	//vals�̃T�C�Y��ۑ�

	//�����ݒ�

	isDie = false;

	//***************�J�n******************
	//	
	//  �c���������_���ɍ쐬���ĂP��ڂƂQ��ڂ͎��ۂ̂ɍ��킹��
	//	���̌ケ�̃^�[���ɗ��Ƃ��c�����i�[
	makeTsumo();
	currentTsumo[0] = tsumo[0];
	currentTsumo[1] = tsumo[1];
	// 
	//  �c�������ۂɗ��Ƃ����Ƃ��ł���ꏊ�݂̂�start��end��ݒ肷��
	//	�t�B�[���h�̍����ɉ����ĂP��u��
	int start;
	int end;
	getFallPosition(start, end, fieldHeight_real);

	//	�c�����~�炷�̂Ɏg�����߁A�������R�s�[���Ă���
	memcpy(fieldHeight_image, fieldHeight_real, sizeof(fieldHeight_real));


	//	start, end�ɉ����ăc�����P���Ƃ����J��Ԃ�
	// �P�藎�Ƃ�
	//	������Ȃ�A�������m�F
	for (; start <= end; start++) {
		memcpy(field_image, field_real, sizeof(field_real));

		fallTsumo(start, currentTsumo);
		chain = tryChain();


		//	�܂�22��vals�Ɋi�[����
		//	12�i�ڂɃc�����������玀��ł���̂ŃX�L�b�v
		if (_mm_testz_si128(field_image[0], die) == 1) {//_mm_testz_si128 => �S��0�̏ꍇ��1
			memcpy(val.reg, field_image, sizeof(field_image));
			val.first = start;
			vals.emplace_back(val);

			//�ő�A�������X�V
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

	//������vals���Ȃ��ꍇ�͒u���Ƃ��낪�Ȃ��@=>�@�ς݂Ȃ̂ŏI���
	if (size = vals.size() == 0) {
		isDie = true;
		return;
	}

	//	�������炳��ɂP��i�߂���J��Ԃ�(vals��*22�ɑ�����̂�RANGE���ŗ}����r�[���T�[�`�T�����s��)
	//	������deep�񐔕��T������
	turn++; //�P��͂��łɐi�񂾂̂�2��ڂ���
	for (; turn < deep; turn++) {

		//	vals�̃T�C�Y���擾
		size = vals.size();

		//	�J�����g�c�����X�V
		currentTsumo[0] = tsumo[(turn * 2) % 128];
		currentTsumo[1] = tsumo[(turn * 2 + 1) % 128];

		//	vals��S�������Ă����S�Ă��P��i�߂� (�ő�RANGE + 22�茩�邱�ƂɂȂ�)
		for (int range = 0; range < size; ++range) {

			//	���u���O�̃t�B�[���h�����R�s�[�Ɏ�������
			memcpy(field_cp, vals[range].reg, sizeof(vals[range].reg));


			//	�������R�s�[�Ɏ�������
			setHeight(field_cp[0], fieldHeight_cp);


			//	start, end���擾
			getFallPosition(start, end, fieldHeight_cp);


			//	�ő�Q�Q�p�^�[���u��
			//	�A��������Δ��΂���
			for (; start <= end; ++start) { //va;ue.reg���畜���H
				//	�]���悤�̍\���̂ɒl��ۑ�
				val.first = vals[range].first; //����

				//	���u���O�̃t�B�[���h���Z�b�g����
				//	�������Z�b�g����
				memcpy(field_image, field_cp, sizeof(field_cp));
				memcpy(fieldHeight_image, fieldHeight_cp, sizeof(fieldHeight_cp));


				//	���u���Ĕ��Ώo�����炷��
				//	�ő�A�����̏���ێ�
				//	val�ɂP�肨������̃t�B�[���h������������
				fallTsumo(start, currentTsumo);

				chain = tryChain();
				if (chainMax < chain) {
					chainMax = chain;
					first = vals[range].first;
				}
				memcpy(val.reg, field_image, sizeof(field_image));


				//	���Ό�̍������擾
				//	�S�F�̃c�����ő�Q�c�ɂU�񂷂ׂĂɒu���Ĕ��΂����߂�
				//		�������u������st, ed���肷��
				setHeight(field_image[0], fieldHeight_image);

				//	����ł�����͎̂����Ȃ�
				if (fieldHeight_image[2] > 11) {
					continue;
				}
				int st;
				int ed;
				getFallPosition(st, ed, fieldHeight_image);
				st = (st + 1) / 4; ed = ed / 4; //pattern => ��ɕϊ�(0�`5)

				//	�ۗL�A������������
				chainMax_ready = 0;
				fireHeight = 0;
				__m128i fallPoint = Thinking::fallPoint; //(1, 1)�������ɂ��� ������P������炷
				int fallCount;

				//	�R�s�[�Ƀt�B�[���h�����������Ă���(���񕜌�����)
				memcpy(field_cp2, field_image, sizeof(field_image));

				//	��񂸂��[�v�Ńc���𗎂Ƃ��Ă���
				for (int row = st; row <= ed; ++row) {
					if (fieldHeight_image[row] > 11) { //12�ȏ�Ȃ�
						fallCount = 0;
					}
					else if (fieldHeight_image[row] > 10) { //11�Ȃ�
						fallCount = 1;
					}
					else {  //10�ȉ��Ȃ�
						fallCount = 2;
					}

					//	�F����F�����Ƃ��Ă���
					for (int color = 1; color <= 4; ++color) {
						//	���Ƃ����������[�v����
						for (int cnt = 0; cnt < fallCount; ++cnt) {
							//fallCount�̐��������Ƃ�
							field_image[color & 0x1] = _mm_or_si128(field_image[color & 0x1], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));
							field_image[((color >> 1) & 0x1) * 2] = _mm_or_si128(field_image[((color >> 1) & 0x1) * 2], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));
							field_image[((color >> 2) & 0x1) * 3] = _mm_or_si128(field_image[((color >> 2) & 0x1) * 3], _mm_srli_epi16(fallPoint, fieldHeight_image[row] + cnt));

							//�ۗL�A���������i�[
							chain = specifiedChain(color);
							if (chain > 0) {
								//���v���Ă���A������ێ�
								if (chain > chainMax_ready) {
									fireHeight = fieldHeight_image[row];
									chainMax_ready = chain;

								}

								//1�ڂŏ�������Q�ڂ͌��Ȃ��Ă����̂�
								break;
							}

						}
						//�t�B�[���h���𕜌�����
						memcpy(field_image, field_cp2, sizeof(field_cp2));
					}
					fallPoint = _mm_srli_si128(fallPoint, 2); //���̗�̈�ԉ�(1 + row, 1)�Ƀr�b�g���ړ�

				}

				//****************�]���l���v�Z(���Ή\�ȘA���� + �E�݂̏��Ȃ�)**************************
				val.score = evalScore(chainMax_ready, fireHeight);

				//�]���l��ۑ����Ĕz��ɒǉ�
				vals.emplace_back(val);

			}

		}

		//�����ŏ��size����폜
		vals.erase(vals.begin(), vals.begin() + size);
		//���(RANGE)���ۑ�
		std::partial_sort(vals.begin(), vals.begin() + (vals.size() < RANGE ? vals.size() : RANGE), vals.end(), std::greater<value>());
		vals.erase(vals.begin() + (vals.size() < RANGE ? vals.size() : RANGE), vals.end());
	}
	pat[first] += chainMax;
	return;
}


//*******************�e�X�g�p*****************

//	reg[]����t�B�[���h���m�F
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

	std::cout << "�`�F�b�N" << std::endl;

}