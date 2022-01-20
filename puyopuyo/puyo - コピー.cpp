// parallel.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "parallel.h"
#include <random>
#include <algorithm>
#include <vector>
#include <iostream>
#include <immintrin.h>
#include <intrin.h>
#include <cstdio>
#include <string>
#include <bitset>
#include <Windows.h>
#include <functional>
#include <time.h>
#include <mmsystem.h>
#include <thread>

#include "Thinking.h";

#pragma comment(lib, "winmm.lib")

//メッセージ
#define DST_LEN (256)
#define MAX_LOADSTRING 100
#define RESET 1
#define WEIGHT 2
#define CHANGE 3
#define NEXT 4
#define DEBUG 5
#define BACK 6
#define CHAIN 7

//フィールド情報
#define FIELD_LEFT 300
#define FIELD_TOP 100
#define TSUMO_SIZE 20

//レーザー探索用
#define RANGE 500 //500手残す
#define DEEP 50  //最大50手探索する


// 探索用の構造体定義
struct val { //評価用
	val() {}
	int value = -10000;
	//int range = 0;
	int first = 0;
	int chain = 0;
	//int cnt = 0;
	__m128i reg1 = _mm_setzero_si128();
	__m128i reg2 = _mm_setzero_si128();
	__m128i reg3 = _mm_setzero_si128();
	//unsigned long long fi[6] = {};
	bool operator>(const val& right) const {
		//return cnt == right.cnt ? value > right.value : cnt > right.cnt;
		return value > right.value;
	}

};

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
HWND hwnd_button_next; //次ボタン
HWND hwnd_button_forward; //前ボタン
HWND hwnd_button_reset;  //リセットボタン
HWND hwnd_button_change;  //変更ボタン
HWND hWnd;
HBRUSH hOldBrush;

std::vector<int> tsumo(128); //配ツモ128手

int turn = 0; //現在のターン
int nowField[8][16]; //今のフィールド状況
int imageField[8][16]; //デバッグ用のイメージフィールド


int controll[512]; //置いた手を保管 => １手戻るよう

//次の手を決めるため
int pat[22];

//死亡判定
bool isArrive = true;

//連鎖処理用のマスク
unsigned int mask[] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000 };

//表示用
int nowChain;

//関数
void paint();
void makeTsumo(std::vector<int>& tsumo);
void fallTsumo2(int field[8][16], std::vector<int> tsumo, int controll);
int fallTsumo(val& valu, unsigned long long a[6], std::vector<int> tsumo, int controll);
void fieldChange(__m128i reg1, __m128i reg2, __m128i reg3, int field[][16]);
bool chain(int field[][16], bool); //連鎖後のフィールドを返す
int tryChain(__m128i* reg1, __m128i* reg2, __m128i* reg3); //連鎖後のフィールドと連鎖数を返す (思考時)
int specifiedChain(__m128i* reg1, __m128i* reg2, __m128i* reg3, int color); //色指定で発火チェック
LPTSTR convertVal(int val); //数値を出力するために変換
void raser(int pat[], int deep); //レーザー探索
void raser2(int pat[], int deep); //レーザー探索試し
void fieldHeight(unsigned long long a[6], int height[]); //フィールドの高さ
int evaluation(volatile int nowHeightArray[6], int chainMax_ready, int fireHeight); //評価値計算
int getAmount();

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: ここにコードを挿入してください。
	//ツモ初期化
	makeTsumo(tsumo);

	// グローバル文字列を初期化する
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_PARALLEL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// アプリケーション初期化の実行:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}
	RECT rx; //ウィンドウ領域
	RECT cx; //クライアント領域
	GetWindowRect(hWnd, &rx);
	GetClientRect(hWnd, &cx);
	const int new_width = 800 + (rx.right - rx.left) - (cx.right - cx.left);
	const int new_height = 600 + (rx.bottom - rx.top) - (cx.bottom - cx.top);
	SetWindowPos(hWnd, NULL, 0, 0, new_width, new_height, SWP_SHOWWINDOW);

	//必要なボタン
	//全部リセット(フィールドと内部情報)
	hwnd_button_reset = CreateWindowA("button", "リセット", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 0, 150, 50, hWnd, (HMENU)RESET, hInstance, NULL);
	//重み変更
	//hwnd_button = CreateWindowA("button", "重み変更", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 50, 150, 50, hWnd, (HMENU)WEIGHT, hInstance, NULL);
	//ツモ変更(全部リセットされる)
	hwnd_button_change = CreateWindowA("button", "ツモ変更", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 100, 150, 50, hWnd, (HMENU)CHANGE, hInstance, NULL);
	//次の手
	hwnd_button_next = CreateWindowA("button", "次の手", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 150, 150, 50, hWnd, (HMENU)NEXT, hInstance, NULL);
	//デバッグ
	//hwnd_button = CreateWindowA("button", "デバッグ", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 200, 150, 50, hWnd, (HMENU)DEBUG, hInstance, NULL);
	//1手戻る
	hwnd_button_forward = CreateWindowA("button", "1手戻る", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 250, 150, 50, hWnd, (HMENU)BACK, hInstance, NULL);
	//連鎖
	//hwnd_button = CreateWindowA("button", "連鎖!", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 650, 300, 150, 50, hWnd, (HMENU)CHAIN, hInstance, NULL);
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PARALLEL));

	MSG msg;
	//タイトル変更
	LPCTSTR title = L"勝手にぷよ";
	SetWindowText(hWnd, title);

	// メイン メッセージ ループ:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//連鎖確認
		while (chain(nowField, true)) {
			//再描画

			std::this_thread::sleep_for(std::chrono::milliseconds(800));
			nowChain++;
			InvalidateRect(hWnd, NULL, false);
			paint();

		}

	}

	return (int)msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PARALLEL));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_PARALLEL);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

	hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 選択されたメニューの解析:
		switch (wmId)
		{
		case RESET: {
			//フィールドリセット
			isArrive = true;
			for (int i = 1; i < 7; i++) {
				for (int j = 1; j < 16; j++) {
					nowField[i][j] = 0;
				}
			}
			//操作手をリセット
			for (int i = 0; i < 512; i++) {
				controll[i] = 0;
			}
			//ターンを0にする
			turn = 0;
			nowChain = 0;

			//再描画
			InvalidateRect(hWnd, NULL, false);
			break;
		}
		case WEIGHT: { // todo2 ) 重みを変えたい
			break;
		}
		case CHANGE: { // todo1 ) 操作手,ターンリセットしたほうがいいかも
			makeTsumo(tsumo);
			InvalidateRect(hWnd, NULL, false);
			break;
		}
		case NEXT: {
			int res = -1; //次の手のパターン（22手）
			int better = 0; //resの最大値取得用

			//ツモの数(探索の深さを調整する)
			// =>フィールドが埋まるほど浅い探索で良いから
			int deep = 6 * 13 - getAmount();
			nowChain = 0;  //表示用

			//思考　５個平行
			//	→大体画面が埋まるまで
			//	→ランダムなツモを用意して２２パターン試して評価値の上位raserパターン次の手を考える
			//	→初手を保存しておいてそれに対応するpta[i]に試した中で最大の連鎖を保存する(平行5個の和になる)


			
			std::thread t1(raser, pat, deep / 2 + 4);
			std::thread t2(raser, pat, deep / 2 + 4);
			std::thread t3(raser, pat, deep / 2 + 4);
			std::thread t4(raser, pat, deep / 2 + 4);
			std::thread t5(raser, pat, deep / 2 + 4);
			std::thread t6(raser, pat, deep / 2 + 4);
			std::thread t7(raser, pat, deep / 2 + 4);
			std::thread t8(raser, pat, deep / 2 + 4);
			std::thread t9(raser, pat, deep / 2 + 4);
			std::thread t10(raser, pat, deep / 2 + 4);

			t1.join(),t2.join(),t3.join(),t4.join(),t5.join(),t6.join(),t7.join(),t8.join(),t9.join(),t10.join();

			//連鎖数の和が最大の手を置く
			if (isArrive) {
				for (int i = 0; i < 22; i++) {
					if (better < pat[i]) {
						better = pat[i];
						res = i;
					}
					pat[i] = 0;
				}

				//操作手格納（戻せるように）
				controll[turn] = res;
				//ターン経過
				turn++;;

				//操作(フィールド変化)
				fallTsumo2(nowField, tsumo, res);
			}
			else {
				//死にましたメッセージ
			}

			//再描画
			InvalidateRect(hWnd, NULL, false);
			paint();

			break;
		}
		case DEBUG: {
			break;
		}
		case BACK: {
			if (turn != 0) {
				isArrive = true; //復活
				turn--;
				//操作手戻す
				controll[turn] = 0;
				//フィールド初期値に戻す
				for (int i = 1; i < 7; i++) {
					for (int j = 1; j < 15; j++) {
						nowField[i][j] = 0;
					}
				}

				//フィールド再構築
				int tmpTurn = turn;
				for (turn = 1; turn <= tmpTurn; turn++) {
					fallTsumo2(nowField, tsumo, controll[turn - 1]);
					chain(nowField, false);
				}
				turn = tmpTurn;
			}
			InvalidateRect(hWnd, NULL, false);
			break;
		}
		case CHAIN: {
			break;
		}
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_PAINT:
	{
		paint();
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

//**************:描画系******************************

//ツモを落とした後の形を取得（描画用）
void fallTsumo2(int field[8][16], std::vector<int> tsumo, int controll) {
	turn--;
	switch (controll) {
	case 0: { //左2回
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 1) % 128];
				field[1][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 1: { //左2回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 0) % 128];
				field[1][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 2: {// X←
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 3: {// Z←←
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 4: { //左1回
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 1) % 128];
				field[2][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 5: { //左1回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 0) % 128];
				field[2][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 6: {// X
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 7: {// Z←
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 8: { //その場
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 1) % 128];
				field[3][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 9: { //その場縦回転
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 0) % 128];
				field[3][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 10: {// X→
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 11: {// Z
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 12: { //右1回
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 1) % 128];
				field[4][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 13: { //右1回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 0) % 128];
				field[4][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 14: {// X→→
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 15: {// Z→
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 16: { //右2回
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 1) % 128];
				field[5][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 17: { // 右2回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 0) % 128];
				field[5][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 18: {// X→→→
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}
	case 19: {// Z→→
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 20: { //右3回
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 1) % 128];
				field[6][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}
	case 21: { //右3回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 0) % 128];
				field[6][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	}
	//14段目の情報を消す
	for (int i = 1; i < 7; i++) {
		for (int k = 1; k < 14; ++k)
			field[i][14] = 0;
	}
	turn++;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

//レジスタから配列に格納
void fieldChange(__m128i reg1, __m128i reg2, __m128i reg3, int field[][16]) {
	uint64_t* llA, * llB, * llC;
	llA = (uint64_t*)&reg1;
	llB = (uint64_t*)&reg2;
	llC = (uint64_t*)&reg3;
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			field[i][j] = 0;
		}
	}
	for (int i = 7; i >= 4; i--) {
		for (int j = 15; j >= 0; j--) {
			field[i][j] ^= (0b1 & llC[0]);
			(field[i][j] <<= 1) ^= (0b1 & llB[0]);
			(field[i][j] <<= 1) ^= (0b1 & llA[0]);
			llA[0] >>= 1;
			llB[0] >>= 1;
			llC[0] >>= 1;
		}
	}
	for (int i = 3; i >= 0; i--) {
		for (int j = 15; j >= 0; j--) {
			field[i][j] ^= (0b1 & llC[1]);
			(field[i][j] <<= 1) ^= (0b1 & llB[1]);
			(field[i][j] <<= 1) ^= (0b1 & llA[1]);
			llA[1] >>= 1;
			llB[1] >>= 1;
			llC[1] >>= 1;
		}
	}
}

//連鎖後のフィールド
bool chain(int field[][16], bool isPaint)
{
	unsigned long long llA, llB, llC;
	llA = llB = llC = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 16; j++) {
			(llA <<= 1) ^= (0b1 & field[i][j]);
			(llB <<= 1) ^= ((0b10 & field[i][j]) >> 1);
			(llC <<= 1) ^= ((0b100 & field[i][j]) >> 2);
		}
	}
	unsigned long long llA2, llB2, llC2;
	llA2 = llB2 = llC2 = 0;
	for (int i = 4; i < 8; i++) {
		for (int j = 0; j < 16; j++) {
			(llA2 <<= 1) ^= (0b1 & field[i][j]);
			(llB2 <<= 1) ^= ((0b10 & field[i][j]) >> 1);
			(llC2 <<= 1) ^= ((0b100 & field[i][j]) >> 2);
		}
	}

	unsigned int mask[] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000 };
	//long long* pData = 0;
	__m128i reg3 = _mm_set_epi64x(llC, llC2);
	__m128i reg2 = _mm_set_epi64x(llB, llB2);
	__m128i reg1 = _mm_set_epi64x(llA, llA2);
	__m128i ojama = _mm_and_si128(reg1, _mm_and_si128(reg2, reg3));
	__m128i colorReg;
	__m128i tmpRegV;
	__m128i tmpRegH;
	__m128i sheedV;
	__m128i sheedH;
	__m128i vanish = _mm_setzero_si128();
	__m128i maskReg;

	int count = 0;
	while (true) {
		for (int color = 1; color < 6; color++) {
			switch (color) {
			case 1: colorReg = _mm_andnot_si128(reg3, _mm_andnot_si128(reg2, reg1)); break;//赤
			case 2: colorReg = _mm_andnot_si128(reg3, _mm_andnot_si128(reg1, reg2)); break;//緑
			case 3: colorReg = _mm_andnot_si128(reg3, _mm_and_si128(reg1, reg2)); break;//青
			case 4: colorReg = _mm_andnot_si128(reg2, _mm_andnot_si128(reg1, reg3)); break;//黄色
			case 5: colorReg = _mm_andnot_si128(reg2, _mm_and_si128(reg1, reg3)); break; //紫
			}
			//std::cout << "上とかぶるやつ" << std::endl;
			tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
			sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
			sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			maskReg = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

			//この段階で0なら次の色でもいい

			colorReg = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128(_mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

			//連結3以上の奴 or 隣接の連結が2以上の連結2の奴
			maskReg = _mm_or_si128(colorReg, maskReg); //連結2いじょうのやつ
			tmpRegV = _mm_and_si128(maskReg, _mm_slli_epi16(maskReg, 1));
			tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			tmpRegH = _mm_and_si128(maskReg, _mm_slli_si128(maskReg, 2));
			tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));

			//シードに隣接する連結1も消える
			tmpRegV = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_epi16(colorReg, 1)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_epi16(colorReg, 1)));
			tmpRegH = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_si128(colorReg, 2)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_si128(colorReg, 2)));
			colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));
			vanish = _mm_or_si128(colorReg, vanish);
		}
		//vanishが0なら抜けたい

		uint64_t* vanishPtr = (uint64_t*)&vanish;
		if ((vanishPtr[0] | vanishPtr[1]) == 0) break;
		count++;
		// ここで上下左右におじゃまがあったらフラグ立てればいいrri
		tmpRegV = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_epi16(vanish, 1)), _mm_and_si128(ojama, _mm_srli_epi16(vanish, 1)));
		tmpRegH = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_si128(vanish, 2)), _mm_and_si128(ojama, _mm_srli_si128(vanish, 2)));
		vanish = _mm_or_si128(vanish, _mm_or_si128(tmpRegV, tmpRegH));
		uint16_t* val = (uint16_t*)&vanish;
		unsigned long a = mask[__popcnt16(val[5])];
		maskReg = _mm_setr_epi16(mask[__popcnt16(val[0])], mask[__popcnt16(val[1])], mask[__popcnt16(val[2])], mask[__popcnt16(val[3])], mask[__popcnt16(val[4])], mask[__popcnt16(val[5])], mask[__popcnt16(val[6])], mask[__popcnt16(val[7])]);
		uint64_t* maskPtr = (uint64_t*)&maskReg;
		colorReg = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
		uint64_t* tmp = (uint64_t*)&colorReg;
		uint64_t* result = (uint64_t*)&reg3;
		reg3 = _mm_set_epi64x(_pdep_u64(_pext_u64(result[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result[0], tmp[0]), maskPtr[0]));
		result = (uint64_t*)&reg2;
		reg2 = _mm_set_epi64x(_pdep_u64(_pext_u64(result[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result[0], tmp[0]), maskPtr[0]));
		result = (uint64_t*)&reg1;
		reg1 = _mm_set_epi64x(_pdep_u64(_pext_u64(result[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result[0], tmp[0]), maskPtr[0]));
		vanish = _mm_setzero_si128();
		if (isPaint) {
			fieldChange(reg1, reg2, reg3, field);
			return true;
		}
	}

	fieldChange(reg1, reg2, reg3, field);
	return false;

}

//フィールド描画
void paint() {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	// TODO: HDC を使用する描画コードをここに追加してください...
	HPEN   hNewPen = (HPEN)CreatePen(PS_INSIDEFRAME, 1, RGB(0x00, 0x00, 0x00));
	HPEN   hOldPen = (HPEN)SelectObject(hdc, hNewPen);
	static HBRUSH redBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0x00, 0x00));
	static HBRUSH greenBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0xFF, 0x7F));
	static HBRUSH blueBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0xBF, 0xFF));
	static HBRUSH yellowBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0xFF, 0x00));
	static HBRUSH purpleBrush = (HBRUSH)CreateSolidBrush(RGB(0x8A, 0x2B, 0xE2));
	static HBRUSH whiteBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
	static HBRUSH blackBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0x00, 0x00));
	hOldBrush = (HBRUSH)SelectObject(hdc, whiteBrush);

	//連鎖数描画
	Rectangle(hdc, 300, 390, 480, 450);
	static LPCWSTR chainStr = L"連鎖数 : ";
	TextOut(hdc, 350, 400, chainStr, lstrlen(chainStr));
	LPCWSTR nowChainStr = convertVal(nowChain);
	TextOut(hdc, 350, 420, nowChainStr, lstrlen(nowChainStr));
	//フィールド描画
	for (int i = 14; i >= 1; i--) {
		for (int j = 1; j < 7; j++) {

			switch (nowField[j][i]) {
			case 1: hOldBrush = (HBRUSH)SelectObject(hdc, redBrush);  break;
			case 2: hOldBrush = (HBRUSH)SelectObject(hdc, greenBrush);  break;
			case 3: hOldBrush = (HBRUSH)SelectObject(hdc, blueBrush);  break;
			case 4: hOldBrush = (HBRUSH)SelectObject(hdc, yellowBrush); break;
			case 7: hOldBrush = (HBRUSH)SelectObject(hdc, blackBrush); break;
			default: hOldBrush = (HBRUSH)SelectObject(hdc, whiteBrush);
			}
			Rectangle(hdc, FIELD_LEFT + (j * 20), FIELD_TOP + ((14 - i) * 20), FIELD_LEFT + (j * 20) + TSUMO_SIZE, FIELD_TOP + ((14 - i) * 20) + TSUMO_SIZE);
		}
	}

	//ツモ描画
	for (int i = 0; i < 2; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdc, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdc, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdc, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdc, yellowBrush); break;
		}
		Rectangle(hdc, FIELD_LEFT + 60, FIELD_TOP - 40 + (i * 20), FIELD_LEFT + 60 + TSUMO_SIZE, FIELD_TOP - 40 + (i * 20) + TSUMO_SIZE);
	}
	for (int i = 2; i < 4; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdc, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdc, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdc, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdc, yellowBrush); break;
		}
		Rectangle(hdc, FIELD_LEFT + 220, FIELD_TOP + (i * 20), FIELD_LEFT + 220 + TSUMO_SIZE, FIELD_TOP + (i * 20) + TSUMO_SIZE);
	}
	for (int i = 4; i < 6; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdc, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdc, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdc, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdc, yellowBrush); break;
		}
		Rectangle(hdc, FIELD_LEFT + 220, FIELD_TOP + 20 + (i * 20), FIELD_LEFT + 220 + TSUMO_SIZE, FIELD_TOP + 20 + (i * 20) + TSUMO_SIZE);
	}
	//DeleteObject(SelectObject(hdc, hOldBrush)); 合った方が良いのか不明
	//DeleteObject(SelectObject(hdc, hOldPen));
	EndPaint(hWnd, &ps);
}

//数値から文字列に変換
LPTSTR convertVal(int val) {
	std::string s = std::to_string(val);
	const char* cs = s.data();
	TCHAR dst[DST_LEN];
	ZeroMemory(&dst[0], DST_LEN);
	MultiByteToWideChar(CP_ACP, 0, &cs[0], sizeof(cs), &dst[0], DST_LEN);
	return dst;
}


//*************思考系********************************

//今あるフィールドのツモの数を取得
int getAmount() {
	int cnt = 0;
	for (int i = 1; i <= 6; i++) {
		for (int k = 1; k <= 13; k++) {
			if (nowField[i][k] > 0) cnt++;
		}
	}
	return cnt;
}

//指定色発火
int specifiedChain(__m128i* reg1, __m128i* reg2, __m128i* reg3, int color) {
	__m128i colorReg;
	__m128i tmpRegV;
	__m128i tmpRegH;
	__m128i sheedV;
	__m128i sheedH;
	__m128i vanish = _mm_setzero_si128();
	__m128i maskReg;

	uint16_t* val = (uint16_t*)&vanish;
	uint64_t* maskPtr = (uint64_t*)&maskReg;
	uint64_t* tmp = (uint64_t*)&colorReg;
	uint64_t* result1 = (uint64_t*)&*reg1;
	uint64_t* result2 = (uint64_t*)&*reg2;
	uint64_t* result3 = (uint64_t*)&*reg3;
	uint64_t* vanishPtr = (uint64_t*)&vanish;

	int count = 0;

	switch (color) {
	case 1: colorReg = _mm_andnot_si128(*reg3, _mm_andnot_si128(*reg2, *reg1)); break;//赤
	case 2: colorReg = _mm_andnot_si128(*reg3, _mm_andnot_si128(*reg1, *reg2)); break;//緑
	case 3: colorReg = _mm_andnot_si128(*reg3, _mm_and_si128(*reg1, *reg2)); break;//青
	case 4: colorReg = _mm_andnot_si128(*reg2, _mm_andnot_si128(*reg1, *reg3)); break;//黄色
	case 5: colorReg = _mm_andnot_si128(*reg2, _mm_and_si128(*reg1, *reg3)); break; //紫
	}
	//std::cout << "上とかぶるやつ" << std::endl;
	tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
	sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
	sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	maskReg = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

	//この段階で0なら次の色でもいい

	colorReg = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128(_mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

	//連結3以上の奴 or 隣接の連結が2以上の連結2の奴
	maskReg = _mm_or_si128(colorReg, maskReg); //連結2いじょうのやつ
	tmpRegV = _mm_and_si128(maskReg, _mm_slli_epi16(maskReg, 1));
	tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
	tmpRegH = _mm_and_si128(maskReg, _mm_slli_si128(maskReg, 2));
	tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
	colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));

	//シードに隣接する連結1も消える
	tmpRegV = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_epi16(colorReg, 1)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_epi16(colorReg, 1)));
	tmpRegH = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_si128(colorReg, 2)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_si128(colorReg, 2)));
	colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));
	vanish = _mm_or_si128(colorReg, vanish);

	//消すものがなければ0を返す
	if ((vanishPtr[0] | vanishPtr[1]) == 0) return 0;

	count++;
	unsigned long a = mask[__popcnt16(val[5])];
	maskReg = _mm_setr_epi16(mask[__popcnt16(val[0])], mask[__popcnt16(val[1])], mask[__popcnt16(val[2])], mask[__popcnt16(val[3])], mask[__popcnt16(val[4])], mask[__popcnt16(val[5])], mask[__popcnt16(val[6])], mask[__popcnt16(val[7])]);
	colorReg = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
	*reg3 = _mm_set_epi64x(_pdep_u64(_pext_u64(result3[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result3[0], tmp[0]), maskPtr[0]));
	*reg2 = _mm_set_epi64x(_pdep_u64(_pext_u64(result2[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result2[0], tmp[0]), maskPtr[0]));
	*reg1 = _mm_set_epi64x(_pdep_u64(_pext_u64(result1[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result1[0], tmp[0]), maskPtr[0]));
	count += tryChain(reg1, reg2, reg3);
	return count;
}

//連鎖数, 連鎖後のフィールド
int tryChain(__m128i* reg1, __m128i* reg2, __m128i* reg3)
{
	//__m128i ojama = _mm_and_si128(*reg1, _mm_and_si128(*reg2, *reg3));
	__m128i colorReg;
	__m128i tmpRegV;
	__m128i tmpRegH;
	__m128i sheedV;
	__m128i sheedH;
	__m128i vanish = _mm_setzero_si128();
	__m128i maskReg;

	uint16_t* val = (uint16_t*)&vanish;
	uint64_t* maskPtr = (uint64_t*)&maskReg;
	uint64_t* tmp = (uint64_t*)&colorReg;
	uint64_t* result1 = (uint64_t*)&*reg1;
	uint64_t* result2 = (uint64_t*)&*reg2;
	uint64_t* result3 = (uint64_t*)&*reg3;
	uint64_t* vanishPtr = (uint64_t*)&vanish;

	int count = 0;
	while (true) {
		for (int color = 1; color < 6; color++) {
			switch (color) {
			case 1: colorReg = _mm_andnot_si128(*reg3, _mm_andnot_si128(*reg2, *reg1)); break;//赤
			case 2: colorReg = _mm_andnot_si128(*reg3, _mm_andnot_si128(*reg1, *reg2)); break;//緑
			case 3: colorReg = _mm_andnot_si128(*reg3, _mm_and_si128(*reg1, *reg2)); break;//青
			case 4: colorReg = _mm_andnot_si128(*reg2, _mm_andnot_si128(*reg1, *reg3)); break;//黄色
			case 5: colorReg = _mm_andnot_si128(*reg2, _mm_and_si128(*reg1, *reg3)); break; //紫
			}
			//std::cout << "上とかぶるやつ" << std::endl;
			tmpRegV = _mm_and_si128(colorReg, _mm_slli_epi16(colorReg, 1));
			sheedV = _mm_xor_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			tmpRegH = _mm_and_si128(colorReg, _mm_slli_si128(colorReg, 2));
			sheedH = _mm_xor_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			maskReg = _mm_or_si128(_mm_and_si128(sheedV, sheedH), _mm_or_si128(_mm_andnot_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_andnot_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

			//この段階で0なら次の色でもいい

			colorReg = _mm_or_si128(_mm_and_si128(_mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2)), _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1))), _mm_or_si128(_mm_and_si128(sheedV, _mm_and_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2))), _mm_and_si128(sheedH, _mm_and_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1)))));

			//連結3以上の奴 or 隣接の連結が2以上の連結2の奴
			maskReg = _mm_or_si128(colorReg, maskReg); //連結2いじょうのやつ
			tmpRegV = _mm_and_si128(maskReg, _mm_slli_epi16(maskReg, 1));
			tmpRegV = _mm_or_si128(tmpRegV, _mm_srli_epi16(tmpRegV, 1));
			tmpRegH = _mm_and_si128(maskReg, _mm_slli_si128(maskReg, 2));
			tmpRegH = _mm_or_si128(tmpRegH, _mm_srli_si128(tmpRegH, 2));
			colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));

			//シードに隣接する連結1も消える
			tmpRegV = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_epi16(colorReg, 1)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_epi16(colorReg, 1)));
			tmpRegH = _mm_or_si128(_mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_slli_si128(colorReg, 2)), _mm_and_si128(_mm_xor_si128(sheedV, sheedH), _mm_srli_si128(colorReg, 2)));
			colorReg = _mm_or_si128(colorReg, _mm_or_si128(tmpRegV, tmpRegH));
			vanish = _mm_or_si128(colorReg, vanish);
		}
		//vanishが0なら抜けたい

		//vanishPtr = (uint64_t*)&vanish;
		if ((vanishPtr[0] | vanishPtr[1]) == 0) break;
		count++;
		// ここで上下左右におじゃまがあったらフラグ立てればいい？(とりあえずお邪魔はなし)
		//tmpRegV = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_epi16(vanish, 1)), _mm_and_si128(ojama, _mm_srli_epi16(vanish, 1)));
		//tmpRegH = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_si128(vanish, 2)), _mm_and_si128(ojama, _mm_srli_si128(vanish, 2)));
		//vanish = _mm_or_si128(vanish, _mm_or_si128(tmpRegV, tmpRegH));

		unsigned long a = mask[__popcnt16(val[5])];
		maskReg = _mm_setr_epi16(mask[__popcnt16(val[0])], mask[__popcnt16(val[1])], mask[__popcnt16(val[2])], mask[__popcnt16(val[3])], mask[__popcnt16(val[4])], mask[__popcnt16(val[5])], mask[__popcnt16(val[6])], mask[__popcnt16(val[7])]);
		colorReg = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
		*reg3 = _mm_set_epi64x(_pdep_u64(_pext_u64(result3[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result3[0], tmp[0]), maskPtr[0]));
		*reg2 = _mm_set_epi64x(_pdep_u64(_pext_u64(result2[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result2[0], tmp[0]), maskPtr[0]));
		*reg1 = _mm_set_epi64x(_pdep_u64(_pext_u64(result1[1], tmp[1]), maskPtr[1]), _pdep_u64(_pext_u64(result1[0], tmp[0]), maskPtr[0]));
		vanish = _mm_setzero_si128();
	}
	return count;

}

//128手均等に色を作る
void makeTsumo(std::vector<int>& tsumo) {
	for (int i = 0; i < 128; i++) {
		tsumo[i] = i / 32 + 1;
	}
	std::random_device get_rand_dev;
	std::mt19937 get_rand_mt(get_rand_dev()); // seedに乱数を指定
	std::shuffle(tsumo.begin(), tsumo.end(), get_rand_mt);
}

//ツモを落とした後の形を取得（次の手考える用）
//	=>ここでvalにregをセットする
int fallTsumo(val& valu, unsigned long long a[6], std::vector<int> tsumo, int controll) {
	int height[6] = {};
	fieldHeight(a, height);
	unsigned long long reg1d = tsumo[1] & 0b1;
	unsigned long long reg2d = tsumo[1] >> 1 & 0b1;
	unsigned long long reg3d = tsumo[1] >> 2 & 0b1;
	unsigned long long reg1u = tsumo[0] & 0b1;
	unsigned long long reg2u = tsumo[0] >> 1 & 0b1;
	unsigned long long reg3u = tsumo[0] >> 2 & 0b1;

	switch (controll) {
	case 0: { //左2回
		if (height[0] > 11) return -1;
		a[0] ^= ((reg1d) << (46 - height[0]));
		a[2] ^= ((reg2d) << (46 - height[0]));
		a[4] ^= ((reg3d) << (46 - height[0]));
		a[0] ^= ((reg1u) << (46 - height[0] - 1));
		a[2] ^= ((reg2u) << (46 - height[0] - 1));
		a[4] ^= ((reg3u) << (46 - height[0] - 1));
		break;
	}
	case 1: { //左2回縦回転
		if (height[0] > 11) return -1;
		a[0] ^= ((reg1u) << (46 - height[0]));
		a[2] ^= ((reg2u) << (46 - height[0]));
		a[4] ^= ((reg3u) << (46 - height[0]));
		a[0] ^= ((reg1d) << (46 - height[0] - 1));
		a[2] ^= ((reg2d) << (46 - height[0] - 1));
		a[4] ^= ((reg3d) << (46 - height[0] - 1));
		break;
	}
	case 2: {// X←
		if (height[0] > 11 || height[1] > 11) return -1;
		a[0] ^= ((reg1d) << (30 - height[1]));
		a[2] ^= ((reg2d) << (30 - height[1]));
		a[4] ^= ((reg3d) << (30 - height[1]));
		a[0] ^= ((reg1u) << (46 - height[0]));
		a[2] ^= ((reg2u) << (46 - height[0]));
		a[4] ^= ((reg3u) << (46 - height[0]));
		break;
	}
	case 3: {// Z←←
		if (height[0] > 11 || height[1] > 11) return -1;
		a[0] ^= ((reg1d) << (46 - height[0]));
		a[2] ^= ((reg2d) << (46 - height[0]));
		a[4] ^= ((reg3d) << (46 - height[0]));
		a[0] ^= ((reg1u) << (30 - height[1]));
		a[2] ^= ((reg2u) << (30 - height[1]));
		a[4] ^= ((reg3u) << (30 - height[1]));
		break;
	}
	case 4: { //左1回
		if (height[1] > 11) return -1;
		a[0] ^= ((reg1d) << (30 - height[1]));
		a[2] ^= ((reg2d) << (30 - height[1]));
		a[4] ^= ((reg3d) << (30 - height[1]));
		a[0] ^= ((reg1u) << (30 - height[1] - 1));
		a[2] ^= ((reg2u) << (30 - height[1] - 1));
		a[4] ^= ((reg3u) << (30 - height[1] - 1));
		break;
	}
	case 5: { //左1回縦回転
		if (height[1] > 11) return -1;
		a[0] ^= ((reg1u) << (30 - height[1]));
		a[2] ^= ((reg2u) << (30 - height[1]));
		a[4] ^= ((reg3u) << (30 - height[1]));
		a[0] ^= ((reg1d) << (30 - height[1] - 1));
		a[2] ^= ((reg2d) << (30 - height[1] - 1));
		a[4] ^= ((reg3d) << (30 - height[1] - 1));
		break;
	}

	case 6: {// X
		if (height[1] > 11 || height[2] > 11) return -1;
		a[0] ^= ((reg1d) << (14 - height[2]));
		a[2] ^= ((reg2d) << (14 - height[2]));
		a[4] ^= ((reg3d) << (14 - height[2]));
		a[0] ^= ((reg1u) << (30 - height[1]));
		a[2] ^= ((reg2u) << (30 - height[1]));
		a[4] ^= ((reg3u) << (30 - height[1]));
		break;
	}
	case 7: {// Z←
		if (height[1] > 11 || height[2] > 11) return -1;
		a[0] ^= ((reg1d) << (30 - height[1]));
		a[2] ^= ((reg2d) << (30 - height[1]));
		a[4] ^= ((reg3d) << (30 - height[1]));
		a[0] ^= ((reg1u) << (14 - height[2]));
		a[2] ^= ((reg2u) << (14 - height[2]));
		a[4] ^= ((reg3u) << (14 - height[2]));
		break;
	}
	case 8: { //その場
		if (height[2] > 11) return -1;
		a[0] ^= ((reg1d) << (14 - height[2]));
		a[2] ^= ((reg2d) << (14 - height[2]));
		a[4] ^= ((reg3d) << (14 - height[2]));
		a[0] ^= ((reg1u) << (14 - height[2] - 1));
		a[2] ^= ((reg2u) << (14 - height[2] - 1));
		a[4] ^= ((reg3u) << (14 - height[2] - 1));
		break;
	}
	case 9: { //その場縦回転
		if (height[2] > 11) return -1;
		a[0] ^= ((reg1u) << (14 - height[2]));
		a[2] ^= ((reg2u) << (14 - height[2]));
		a[4] ^= ((reg3u) << (14 - height[2]));
		a[0] ^= ((reg1d) << (14 - height[2] - 1));
		a[2] ^= ((reg2d) << (14 - height[2] - 1));
		a[4] ^= ((reg3d) << (14 - height[2] - 1));
		break;
	}

	case 10: {// X→
		if (height[2] > 11 || height[3] > 11) return -1;
		a[1] ^= ((reg1d) << (62 - height[3]));
		a[3] ^= ((reg2d) << (62 - height[3]));
		a[5] ^= ((reg3d) << (62 - height[3]));
		a[0] ^= ((reg1u) << (14 - height[2]));
		a[2] ^= ((reg2u) << (14 - height[2]));
		a[4] ^= ((reg3u) << (14 - height[2]));
		break;
	}
	case 11: {// Z
		if (height[2] > 11 || height[3] > 11) return -1;
		a[0] ^= ((reg1d) << (14 - height[2]));
		a[2] ^= ((reg2d) << (14 - height[2]));
		a[4] ^= ((reg3d) << (14 - height[2]));
		a[1] ^= ((reg1u) << (62 - height[3]));
		a[3] ^= ((reg2u) << (62 - height[3]));
		a[5] ^= ((reg3u) << (62 - height[3]));
		break;
	}
	case 12: { //右1回
		if (height[3] > 11) return -1;
		a[1] ^= ((reg1d) << (62 - height[3]));
		a[3] ^= ((reg2d) << (62 - height[3]));
		a[5] ^= ((reg3d) << (62 - height[3]));
		a[1] ^= ((reg1u) << (62 - height[3] - 1));
		a[3] ^= ((reg2u) << (62 - height[3] - 1));
		a[5] ^= ((reg3u) << (62 - height[3] - 1));
		break;
	}
	case 13: { //右1回縦回転
		if (height[3] > 11) return -1;
		a[1] ^= ((reg1u) << (62 - height[3]));
		a[3] ^= ((reg2u) << (62 - height[3]));
		a[5] ^= ((reg3u) << (62 - height[3]));
		a[1] ^= ((reg1d) << (62 - height[3] - 1));
		a[3] ^= ((reg2d) << (62 - height[3] - 1));
		a[5] ^= ((reg3d) << (62 - height[3] - 1));
		break;
	}


	case 14: {// X→→
		if (height[3] > 11 || height[4] > 11) return -1;
		a[1] ^= ((reg1d) << (46 - height[4]));
		a[3] ^= ((reg2d) << (46 - height[4]));
		a[5] ^= ((reg3d) << (46 - height[4]));
		a[1] ^= ((reg1u) << (62 - height[3]));
		a[3] ^= ((reg2u) << (62 - height[3]));
		a[5] ^= ((reg3u) << (62 - height[3]));
		break;
	}
	case 15: {// Z→
		if (height[3] > 11 || height[4] > 11) return -1;
		a[1] ^= ((reg1d) << (62 - height[3]));
		a[3] ^= ((reg2d) << (62 - height[3]));
		a[5] ^= ((reg3d) << (62 - height[3]));
		a[1] ^= ((reg1u) << (46 - height[4]));
		a[3] ^= ((reg2u) << (46 - height[4]));
		a[5] ^= ((reg3u) << (46 - height[4]));
		break;
	}

	case 16: { //右2回
		if (height[4] > 11) return -1;
		a[1] ^= ((reg1d) << (46 - height[4]));
		a[3] ^= ((reg2d) << (46 - height[4]));
		a[5] ^= ((reg3d) << (46 - height[4]));
		a[1] ^= ((reg1u) << (46 - height[4] - 1));
		a[3] ^= ((reg2u) << (46 - height[4] - 1));
		a[5] ^= ((reg3u) << (46 - height[4] - 1));
		break;
	}
	case 17: { // 右2回縦回転
		if (height[4] > 11) return -1;
		a[1] ^= ((reg1u) << (46 - height[4]));
		a[3] ^= ((reg2u) << (46 - height[4]));
		a[5] ^= ((reg3u) << (46 - height[4]));
		a[1] ^= ((reg1d) << (46 - height[4] - 1));
		a[3] ^= ((reg2d) << (46 - height[4] - 1));
		a[5] ^= ((reg3d) << (46 - height[4] - 1));
		break;
	}
	case 18: {// X→→→
		if (height[4] > 11 || height[5] > 11) return -1;
		a[1] ^= ((reg1d) << (30 - height[5]));
		a[3] ^= ((reg2d) << (30 - height[5]));
		a[5] ^= ((reg3d) << (30 - height[5]));
		a[1] ^= ((reg1u) << (46 - height[4]));
		a[3] ^= ((reg2u) << (46 - height[4]));
		a[5] ^= ((reg3u) << (46 - height[4]));
		break;
	}
	case 19: {// Z→→
		if (height[4] > 11 || height[5] > 11) return -1;
		a[1] ^= ((reg1d) << (46 - height[4]));
		a[3] ^= ((reg2d) << (46 - height[4]));
		a[5] ^= ((reg3d) << (46 - height[4]));
		a[1] ^= ((reg1u) << (30 - height[5]));
		a[3] ^= ((reg2u) << (30 - height[5]));
		a[5] ^= ((reg3u) << (30 - height[5]));
		break;
	}
	case 20: { //右3回
		if (height[5] > 11) return -1;
		a[1] ^= ((reg1d) << (30 - height[5]));
		a[3] ^= ((reg2d) << (30 - height[5]));
		a[5] ^= ((reg3d) << (30 - height[5]));
		a[1] ^= ((reg1u) << (30 - height[5] - 1));
		a[3] ^= ((reg2u) << (30 - height[5] - 1));
		a[5] ^= ((reg3u) << (30 - height[5] - 1));
		break;
	}
	case 21: { //右3回縦回転
		if (height[5] > 11) return -1;
		a[1] ^= ((reg1u) << (30 - height[5]));
		a[3] ^= ((reg2u) << (30 - height[5]));
		a[5] ^= ((reg3u) << (30 - height[5]));
		a[1] ^= ((reg1d) << (30 - height[5] - 1));
		a[3] ^= ((reg2d) << (30 - height[5] - 1));
		a[5] ^= ((reg3d) << (30 - height[5] - 1));
		break;
	}


	}
	//14段目の情報を消す
	__m128i fourteen_vanish = _mm_set_epi64x(0x0000000200020002, 0x0002000200020000); //14段目のビットが立っている
	valu.reg1 = _mm_andnot_si128(fourteen_vanish, _mm_set_epi64x(a[0], a[1]));
	valu.reg2 = _mm_andnot_si128(fourteen_vanish, _mm_set_epi64x(a[2], a[3]));
	valu.reg3 = _mm_andnot_si128(fourteen_vanish, _mm_set_epi64x(a[4], a[5]));

	return 0;
}

//フィールドの高さを取得
void fieldHeight(unsigned long long a[6], int height[]) {
	unsigned long long llA = a[0], llB = a[2], llC = a[4];
	unsigned long long llA2 = a[1], llB2 = a[3], llC2 = a[5];
	__m128i reg3 = _mm_set_epi64x(llC, llC2);
	__m128i reg2 = _mm_set_epi64x(llB, llB2);
	__m128i reg1 = _mm_set_epi64x(llA, llA2);
	__m128i reg = _mm_or_si128(reg3, _mm_or_si128(reg2, reg1));
	uint16_t* val = (uint16_t*)&reg;
	for (int i = 0; i < 6; i++) {
		height[i] = __popcnt16(val[6 - i]);
	}
	return;
}


//思考部分
void raser(int pat[], int deep) {
	//vector関係
	std::vector<val> value;
	value.reserve(10000);
	val valu;

	int chainMax = 0; //関数内の最大連鎖数
	int chain;  //連鎖数保持
	int chainMax_ready = 0; //聴牌している連鎖数
	int chain_already = 0;//必要になる（valにもたせたい）

	volatile int first = -1; //最大連鎖をしたときの初手を格納する(return時に使用)
	int turnM = turn;//何turn目かを保存しておく
	volatile int count = 0; //valu.size格納(loop回数)
	std::vector<int> tsumoM(128);

	//配列コピー用
	unsigned long long tmpfield[6] = {};
	unsigned long long tmpfield_cp[6] = {};

	//評価用
	volatile int height[6] = {};
	volatile int nowHeightArray[6] = {};
	int evalVal = 0;
	int fireHeight = 0;
	int heightAve = 0;
	int heightValue = 0;

	//こつも //おやつも
	int tumo[2];

	//ツモを置く位置
	int start;
	int last;
	int st;
	int ed;

	//汎用
	__m128i fourteen_vanish = _mm_set_epi64x(0x0000000200020002, 0x0002000200020000); //14段目のビットが立っている
	__m128i die = _mm_set_epi64x(0x8, 0x0); //3列目12段目のビットが立っている
	__m128i fourteen; //14段目の情報
	__m128i reg; //いろいろ使う


	//現在のフィールド
	__m128i reg1 = _mm_setzero_si128();
	__m128i reg2 = _mm_setzero_si128();
	__m128i reg3 = _mm_setzero_si128();


	//1手先のフィールド
	__m128i reg2_1;
	__m128i reg2_2;
	__m128i reg2_3;


	//発火を試すフィールド
	__m128i reg3_1;
	__m128i reg3_2;
	__m128i reg3_3;

	//発火を試すときに落とすツモ
	__m128i fallPoint1;
	__m128i fallPoint2;
	__m128i fallPoint = _mm_set_epi64x(0x0000400000000000, 0x0); //(1, 1)のビットが立ってる

	//高さ関連
	__m128i nowHeight;
	uint16_t* heightPtr = (uint16_t*)&reg;
	uint16_t* nowHeightPtr = (uint16_t*)&nowHeight;



	int fallCount; //発火を試すためにツモを落とす個数

	//std::cout << "2の位置" << std::endl;


	//ランダムに手を作る
	makeTsumo(tsumoM);
	//初手、次の手を変更
	for (int t = 0; t < 4; t++) {
		tsumoM[t] = tsumo[(t + (turn * 2)) % 128];
	}
	//最初はループ外
	//turn = 0;
	//配列にコピー
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
	reg1 = _mm_set_epi64x(tmpfield[0], tmpfield[1]);
	reg2 = _mm_set_epi64x(tmpfield[2], tmpfield[3]);
	reg3 = _mm_set_epi64x(tmpfield[4], tmpfield[5]);
	reg = _mm_or_si128(reg3, _mm_or_si128(reg2, reg1));
	for (int i = 0; i < 6; i++) {
		height[i] = __popcnt16(heightPtr[6 - i]);
	}
	//1パターン置く(おけないところには置かない)
	if (height[1] < 13) {
		if (height[0] < 13) {
			start = 0;
		}
		else if (height[0] == 13) {
			start = 2;
		}
		else {
			start = 4;
		}
	}
	else {
		start = 8;
	}

	if (height[3] < 13) {
		if (height[4] < 13) {
			if (height[5] < 13) {
				last = 21;
			}
			else if (height[5] == 13) {
				last = 19;
			}
			else {
				last = 17;
			}
		}
		else if (height[4] == 12 && height[5] < 13) {
			last = 21;
		}
		else {
			last = 13;
		}
	}
	else {
		last = 9;
	}
	//14段目を消す
	//reg = _mm_andnot_si128(forteen, _mm_or_si128(reg3, _mm_or_si128(reg2, reg1)));

	for (int pattern = start; pattern <= last; pattern++) {
		memcpy(tmpfield_cp, tmpfield, sizeof(tmpfield));
		valu.first = pattern;

		//ツモを落として
		//valu.reg1 = reg1;
		//valu.reg2 = reg2;
		//valu.reg3 = reg3;を行う
		fallTsumo(valu, tmpfield_cp, tsumoM, pattern);

		//連鎖させて何連鎖かを格納する
		chain = tryChain(&valu.reg1, &valu.reg2, &valu.reg3);
		valu.chain = chain; //発火を保存

		if (chainMax < chain) {
			chainMax = chain;
			first = pattern;
		}
		//12段目にあったらなしにする
		reg = _mm_or_si128(valu.reg3, _mm_or_si128(valu.reg2, valu.reg1));
		if (_mm_testz_si128(reg, die) == 1) {//全て0の場合
			value.emplace_back(valu);
		}
		else {
			continue;
		}

	}

	//なにもなければfalseを返す(死亡判定)
	if (value.size() < 1) {
		isArrive = false;
		return;
	}

	//deep回ループ
	for (int i = 1; i < deep; ++i) {
		turnM = i;
		count = value.size();
		//RANGE * 22パターン ここからのを関数に頼らず行えばちょい早い
		for (volatile int range = 0; range < count; ++range) {

			//評価値を初期化
			value[range].value = -10000;
			//レジスタに手を置く前のフィールド情報を格納
			reg1 = value[range].reg1;
			reg2 = value[range].reg2;
			reg3 = value[range].reg3;

			//高さ格納(14段目は無視)
			reg = _mm_andnot_si128(fourteen_vanish, _mm_or_si128(reg3, _mm_or_si128(reg2, reg1)));
			for (int i = 0; i < 6; i++) {
				height[i] = __popcnt16(heightPtr[6 - i]);
			}
			//カレントツモ
			tumo[0] = tsumoM[turnM * 2 % 128 + 0]; //こ
			tumo[1] = tsumoM[turnM * 2 % 128 + 1]; //おや

			//patternの幅を変化させる
			if (height[1] < 13) {
				if (height[0] < 13) {
					start = 0;
				}
				else if (height[0] == 13) {
					start = 2;
				}
				else {
					start = 4;
				}
			}
			else {
				start = 8;
			}

			if (height[3] < 13) {
				if (height[4] < 13) {
					if (height[5] < 13) {
						last = 21;
					}
					else if (height[5] == 13) {
						last = 19;
					}
					else {
						last = 17;
					}
				}
				else if (height[4] == 12 && height[5] < 13) {
					last = 21;
				}
				else {
					last = 13;
				}
			}
			else {
				last = 9;
			}

			//最大２２パターン置く
			for (; start <= last; ++start) {
				//評価ようの構造体に値を保存
				valu.first = value[range].first; //初手
				valu.chain = value[range].chain;

				//1パターンずつ置く

				//　そのまま落下時の上が〇下が×とすると
				//　　０　１　２　　３
				//　　〇　×　
				//　　×　〇　〇×　×〇
				//　という順で２２手置く

				//	fallpoint <- (1, 1)の位置
				//	fallpoint1 <- 縦置き時の上側と横置き時の左側　０～３は１列目４～７は２列目
				//	fallpoint2 <- 縦置き時の下側と横置き時の右側　２～５は２列目６～１０は３列目

				//switch (start / 4) {
				//case 0: fallPoint1 = fallPoint; break;
				//case 1: fallPoint1 = _mm_srli_si128(fallPoint, 2); break;
				//case 2: fallPoint1 = _mm_srli_si128(fallPoint, 4); break;
				//case 3: fallPoint1 = _mm_srli_si128(fallPoint, 6); break;
				//case 4: fallPoint1 = _mm_srli_si128(fallPoint, 8); break;
				//case 5: fallPoint1 = _mm_srli_si128(fallPoint, 10); break;
				//}
				
				//switch ((start + 2) / 4) {
				//case 0: fallPoint2 = fallPoint; break;
				//case 1: fallPoint2 = _mm_srli_si128(fallPoint, 2); break;
				//case 2: fallPoint2 = _mm_srli_si128(fallPoint, 4); break;
				//case 3: fallPoint2 = _mm_srli_si128(fallPoint, 6); break;
				//case 4: fallPoint2 = _mm_srli_si128(fallPoint, 8); break;
				//case 5: fallPoint2 = _mm_srli_si128(fallPoint, 10); break;
				//}

				fallPoint1 = _mm_srli_epi16(_mm_srli_si128(fallPoint, 2 * (start / 4)), height[start / 4] + (((start + 2) / 2) % 2));
				fallPoint2 = _mm_srli_epi16(_mm_srli_si128(fallPoint, 2 * ((start + 2) / 4)), height[(start + 2) / 4]);

				if (tumo[start % 2] & 0x1) {
					reg2_1 = _mm_or_si128(reg1, fallPoint1); //上と左
				}
				else {
					reg2_1 = reg1;
				}
				if ((tumo[start % 2] >> 1) & 0x1) {
					reg2_2 = _mm_or_si128(reg2, fallPoint1);
				}
				else {
					reg2_2 = reg2;
				}
				if ((tumo[start % 2] >> 2) & 0x1) {
					reg2_3 = _mm_or_si128(reg3, fallPoint1);
				}
				else {
					reg2_3 = reg3;
				}
				//fallPointを決めてあげればいい
				if (tumo[(start + 1) % 2] & 0x1) {
					reg2_1 = _mm_or_si128(reg2_1, fallPoint2); //下と右
				}
				if ((tumo[(start + 1) % 2] >> 1) & 0x1) {
					reg2_2 = _mm_or_si128(reg2_2, fallPoint2);
				}
				if ((tumo[(start + 1) % 2] >> 2) & 0x1) {
					reg2_3 = _mm_or_si128(reg2_3, fallPoint2);
				}

				//発火できたらする
				chain = 0;
				//ここで14段目消滅
				fourteen = _mm_and_si128(_mm_or_si128(reg2_3, _mm_or_si128(reg2_2, reg2_1)), fourteen_vanish); //14段目の情報
				reg2_1 = _mm_andnot_si128(fourteen_vanish, reg2_1);
				reg2_2 = _mm_andnot_si128(fourteen_vanish, reg2_2);
				reg2_3 = _mm_andnot_si128(fourteen_vanish, reg2_3);

				chain = tryChain(&reg2_1, &reg2_2, &reg2_3);
				if (chain > 0) {
					if (chain > value[range].chain) {
						valu.chain = chain;
					}
					if (chainMax < chain) {
						chainMax = chain;
						first = value[range].first;

					}
				}

				//発火後の高さで
				nowHeight = _mm_or_si128(reg2_3, _mm_or_si128(reg2_2, reg2_1));
				for (int i = 0; i < 6; i++) {
					nowHeightArray[i] = __popcnt16(nowHeightPtr[6 - i]);
				}

				//死んでいるものは試さない
				if (nowHeightArray[2] > 11) {
					continue;
				}

				//超えれない場所は試さない
				if (nowHeightArray[1] < 13) {
					if (nowHeightArray[0] < 13) {
						st = 0;
					}
					else {
						st = 1;
					}
				}
				else {
					st = 2;
				}

				if (nowHeightArray[3] < 13) {
					if (nowHeightArray[4] < 13) {
						if (nowHeightArray[5] < 13) {
							ed = 5;
						}
						else {
							ed = 4;
						}
					}
					else {
						ed = 3;
					}
				}
				else {
					ed = 2;
				}

				//ツモを落として発火させる
				// 
				//聴牌連鎖数を初期化
				chainMax_ready = 0;
				fallPoint1 = _mm_set_epi64x(0x0000400000000000, 0x0);
				for (int ts = st; ts <= ed; ++ts) {

					if (nowHeightArray[ts] > 11) { //12個以上なら
						fallCount = 0;
					}
					else if (nowHeightArray[ts] > 10) { //11個なら
						fallCount = 1;
					}
					else {  //10個以下なら
						fallCount = 2;
					}

					//色を一色ずつ落としていく
					for (int i = 1; i <= 4; ++i) {
						reg3_1 = reg2_1;
						reg3_2 = reg2_2;
						reg3_3 = reg2_3;
						for (int k = 0; k < fallCount; ++k) {
							//countの数だけ落とす
							if (i & 0x1) {
								reg3_1 = _mm_or_si128(reg3_1, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}
							if ((i >> 1) & 0x1) {
								reg3_2 = _mm_or_si128(reg3_2, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}
							if ((i >> 2) & 0x1) {
								reg3_3 = _mm_or_si128(reg3_3, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}

							//連鎖数をセット
							chain = specifiedChain(&reg3_1, &reg3_2, &reg3_3, i);

							if (chain > 0) {
								//聴牌している連鎖数を保持
								if (chain > chainMax_ready) {
									fireHeight = nowHeightArray[ts];
									chainMax_ready = chain;

									if (chain > value[range].chain) {
										valu.chain = chain;
									}

								}

								//最大だったら入れ替える
								if (chainMax < chain) {
									chainMax = chain;
									first = value[range].first;
								}
								//1個で消えたら２個目は見なくていいのでkをインクリメント
								k++;
								break;
							}
						}
					}
					fallPoint1 = _mm_srli_si128(fallPoint1, 2); //自分の列の一番下(j+1, 1)にビットを移動

				}

				//****************評価値を計算(発火可能な連鎖数 + 窪みの少なさ)**************************
				evalVal = evaluation(nowHeightArray, chainMax_ready, fireHeight);

				//ここで14段目復活して格納(今回は復活無意味)
				valu.reg1 = _mm_or_si128(fourteen, reg2_1);
				valu.reg2 = _mm_or_si128(fourteen, reg2_2);
				valu.reg3 = _mm_or_si128(fourteen, reg2_3);

				//評価値を保存して配列に追加
				valu.value = evalVal;
				value.emplace_back(valu);
			}
		}

		//上位(RANGE)手を保存
		std::partial_sort(value.begin(), value.begin() + (count < RANGE ? count : RANGE), value.end(), std::greater<val>());
		value.erase(value.begin() + (count < RANGE ? count : RANGE), value.end());
	}
	//fieldChange(value[0].reg1, value[0].reg2, value[0].reg3, nowField);
	pat[first] += chainMax;
	return;
}

void raser2(int pat[], int deep) {
	//vector関係
	std::vector<val> value;
	value.reserve(10000);
	val valu;

	int chainMax[22] = {}; //関数内の最大連鎖数
	int chain;  //連鎖数保持
	int chainMax_ready{}; //聴牌している連鎖数


	volatile int first = -1; //最大連鎖をしたときの初手を格納する(return時に使用)
	int turnM = turn;//何turn目かを保存しておく
	volatile int count = 0; //valu.size格納(loop回数)
	std::vector<int> tsumoM(128);

	//配列コピー用
	unsigned long long tmpfield[6] = {};
	unsigned long long tmpfield_cp[6] = {};

	//評価用
	volatile int height[6] = {};
	volatile int nowHeightArray[6] = {};
	int evalVal = 0;
	int fireHeight = 0;
	int heightAve = 0;
	int heightValue = 0;

	//こつも //おやつも
	int tumo[2];

	//ツモを置く位置
	int start;
	int last;
	int st;
	int ed;

	//汎用
	__m128i fourteen_vanish = _mm_set_epi64x(0x0000000200020002, 0x0002000200020000); //14段目のビットが立っている
	__m128i die = _mm_set_epi64x(0x8, 0x0); //3列目12段目のビットが立っている
	__m128i fourteen; //14段目の情報
	__m128i reg; //いろいろ使う


	//現在のフィールド
	__m128i reg1 = _mm_setzero_si128();
	__m128i reg2 = _mm_setzero_si128();
	__m128i reg3 = _mm_setzero_si128();


	//1手先のフィールド
	__m128i reg2_1;
	__m128i reg2_2;
	__m128i reg2_3;


	//発火を試すフィールド
	__m128i reg3_1;
	__m128i reg3_2;
	__m128i reg3_3;

	//発火を試すときに落とすツモ
	__m128i fallPoint1;
	__m128i fallPoint2;
	__m128i fallPoint = _mm_set_epi64x(0x0000400000000000, 0x0); //(1, 1)のビットが立ってる

	//高さ関連
	__m128i nowHeight;
	uint16_t* heightPtr = (uint16_t*)&reg;
	uint16_t* nowHeightPtr = (uint16_t*)&nowHeight;



	int fallCount; //発火を試すためにツモを落とす個数

	//std::cout << "2の位置" << std::endl;


	//ランダムに手を作る
	makeTsumo(tsumoM);
	//初手、次の手を変更
	for (int t = 0; t < 4; t++) {
		tsumoM[t] = tsumo[(t + (turn * 2)) % 128];
	}
	//最初はループ外
	//turn = 0;
	//配列にコピー
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
	reg1 = _mm_set_epi64x(tmpfield[0], tmpfield[1]);
	reg2 = _mm_set_epi64x(tmpfield[2], tmpfield[3]);
	reg3 = _mm_set_epi64x(tmpfield[4], tmpfield[5]);
	reg = _mm_or_si128(reg3, _mm_or_si128(reg2, reg1));
	for (int i = 0; i < 6; i++) {
		height[i] = __popcnt16(heightPtr[6 - i]);
	}
	//1パターン置く(おけないところには置かない)
	if (height[1] < 13) {
		if (height[0] < 13) {
			start = 0;
		}
		else if (height[0] == 13) {
			start = 2;
		}
		else {
			start = 4;
		}
	}
	else {
		start = 8;
	}

	if (height[3] < 13) {
		if (height[4] < 13) {
			if (height[5] < 13) {
				last = 21;
			}
			else if (height[5] == 13) {
				last = 19;
			}
			else {
				last = 17;
			}
		}
		else if (height[4] == 12 && height[5] < 13) {
			last = 21;
		}
		else {
			last = 13;
		}
	}
	else {
		last = 9;
	}
	//14段目を消す
	//reg = _mm_andnot_si128(forteen, _mm_or_si128(reg3, _mm_or_si128(reg2, reg1)));

	for (int pattern = start; pattern <= last; pattern++) {
		memcpy(tmpfield_cp, tmpfield, sizeof(tmpfield));
		valu.first = pattern;

		//ツモを落として
		//valu.reg1 = reg1;
		//valu.reg2 = reg2;
		//valu.reg3 = reg3;を行う
		fallTsumo(valu, tmpfield_cp, tsumoM, pattern);

		//連鎖させて何連鎖かを格納する
		chain = tryChain(&valu.reg1, &valu.reg2, &valu.reg3);
		if (chainMax[pattern] < chain) {
			chainMax[pattern] = chain;
			//first = pattern;
		}
		//12段目にあったらなしにする
		reg = _mm_or_si128(valu.reg3, _mm_or_si128(valu.reg2, valu.reg1));
		if (_mm_testz_si128(reg, die) == 1) {//全て0の場合
			value.emplace_back(valu);
		}
		else {
			continue;
		}

	}

	//なにもなければfalseを返す(死亡判定)
	if (value.size() < 1) {
		isArrive = false;
		return;
	}

	//deep回ループ
	for (int i = 1; i < deep; ++i) {
		turnM = i;
		count = value.size();
		//RANGE * 22パターン ここからのを関数に頼らず行えばちょい早い
		for (volatile int range = 0; range < count; ++range) {

			//評価値を初期化
			value[range].value = -10000;
			//レジスタに手を置く前のフィールド情報を格納
			reg1 = value[range].reg1;
			reg2 = value[range].reg2;
			reg3 = value[range].reg3;

			//高さ格納(14段目は無視)
			reg = _mm_andnot_si128(fourteen_vanish, _mm_or_si128(reg3, _mm_or_si128(reg2, reg1)));
			for (int i = 0; i < 6; i++) {
				height[i] = __popcnt16(heightPtr[6 - i]);
			}
			//カレントツモ
			tumo[0] = tsumoM[turnM * 2 % 128 + 0]; //こ
			tumo[1] = tsumoM[turnM * 2 % 128 + 1]; //おや

			//patternの幅を変化させる
			if (height[1] < 13) {
				if (height[0] < 13) {
					start = 0;
				}
				else if (height[0] == 13) {
					start = 2;
				}
				else {
					start = 4;
				}
			}
			else {
				start = 8;
			}

			if (height[3] < 13) {
				if (height[4] < 13) {
					if (height[5] < 13) {
						last = 21;
					}
					else if (height[5] == 13) {
						last = 19;
					}
					else {
						last = 17;
					}
				}
				else if (height[4] == 12 && height[5] < 13) {
					last = 21;
				}
				else {
					last = 13;
				}
			}
			else {
				last = 9;
			}

			//最大２２パターン置く
			for (; start <= last; ++start) {
				//評価ようの構造体に値を保存
				valu.first = value[range].first; //初手

				//1パターン置く
				switch (start / 4) {
				case 0: fallPoint1 = fallPoint; break;
				case 1: fallPoint1 = _mm_srli_si128(fallPoint, 2); break;
				case 2: fallPoint1 = _mm_srli_si128(fallPoint, 4); break;
				case 3: fallPoint1 = _mm_srli_si128(fallPoint, 6); break;
				case 4: fallPoint1 = _mm_srli_si128(fallPoint, 8); break;
				case 5: fallPoint1 = _mm_srli_si128(fallPoint, 10); break;
				}
				switch ((start + 2) / 4) {
				case 0: fallPoint2 = fallPoint; break;
				case 1: fallPoint2 = _mm_srli_si128(fallPoint, 2); break;
				case 2: fallPoint2 = _mm_srli_si128(fallPoint, 4); break;
				case 3: fallPoint2 = _mm_srli_si128(fallPoint, 6); break;
				case 4: fallPoint2 = _mm_srli_si128(fallPoint, 8); break;
				case 5: fallPoint2 = _mm_srli_si128(fallPoint, 10); break;
				}
				fallPoint1 = _mm_srli_epi16(fallPoint1, height[start / 4] + (((start + 2) / 2) % 2));
				fallPoint2 = _mm_srli_epi16(fallPoint2, height[(start + 2) / 4]);
				if (tumo[start % 2] & 0x1) {
					reg2_1 = _mm_or_si128(reg1, fallPoint1); //上と左
				}
				else {
					reg2_1 = reg1;
				}
				if ((tumo[start % 2] >> 1) & 0x1) {
					reg2_2 = _mm_or_si128(reg2, fallPoint1);
				}
				else {
					reg2_2 = reg2;
				}
				if ((tumo[start % 2] >> 2) & 0x1) {
					reg2_3 = _mm_or_si128(reg3, fallPoint1);
				}
				else {
					reg2_3 = reg3;
				}
				//fallPointを決めてあげればいい
				if (tumo[(start + 1) % 2] & 0x1) {
					reg2_1 = _mm_or_si128(reg2_1, fallPoint2); //下と右
				}
				if ((tumo[(start + 1) % 2] >> 1) & 0x1) {
					reg2_2 = _mm_or_si128(reg2_2, fallPoint2);
				}
				if ((tumo[(start + 1) % 2] >> 2) & 0x1) {
					reg2_3 = _mm_or_si128(reg2_3, fallPoint2);
				}

				//発火できたらする
				chain = 0;
				//ここで14段目消滅
				fourteen = _mm_and_si128(_mm_or_si128(reg2_3, _mm_or_si128(reg2_2, reg2_1)), fourteen_vanish); //14段目の情報
				reg2_1 = _mm_andnot_si128(fourteen_vanish, reg2_1);
				reg2_2 = _mm_andnot_si128(fourteen_vanish, reg2_2);
				reg2_3 = _mm_andnot_si128(fourteen_vanish, reg2_3);

				chain = tryChain(&reg2_1, &reg2_2, &reg2_3);
				if (chainMax[value[range].first] < chain) {
					chainMax[value[range].first] = chain;
					//first = value[range].first;
				}

				//発火後の高さで
				nowHeight = _mm_or_si128(reg2_3, _mm_or_si128(reg2_2, reg2_1));
				for (int i = 0; i < 6; i++) {
					nowHeightArray[i] = __popcnt16(nowHeightPtr[6 - i]);
				}

				//死んでいるものは試さない
				if (nowHeightArray[2] > 11) {
					continue;
				}

				//超えれない場所は試さない
				if (nowHeightArray[1] < 13) {
					if (nowHeightArray[0] < 13) {
						st = 0;
					}
					else {
						st = 1;
					}
				}
				else {
					st = 2;
				}

				if (nowHeightArray[3] < 13) {
					if (nowHeightArray[4] < 13) {
						if (nowHeightArray[5] < 13) {
							ed = 5;
						}
						else {
							ed = 4;
						}
					}
					else {
						ed = 3;
					}
				}
				else {
					ed = 2;
				}

				//ツモを落として発火させる
				//
				//聴牌連鎖数を初期化
				chainMax_ready = 0;
				fallPoint1 = _mm_set_epi64x(0x0000400000000000, 0x0);
				for (int ts = st; ts <= ed; ++ts) {

					if (nowHeightArray[ts] > 11) { //12個以上なら
						fallCount = 0;
					}
					else if (nowHeightArray[ts] > 10) { //11個なら
						fallCount = 1;
					}
					else {  //10個以下なら
						fallCount = 2;
					}

					//色を一色ずつ落としていく
					for (int i = 1; i <= 4; ++i) {
						reg3_1 = reg2_1;
						reg3_2 = reg2_2;
						reg3_3 = reg2_3;
						for (int k = 0; k < fallCount; ++k) {
							//countの数だけ落とす
							if (i & 0x1) {
								reg3_1 = _mm_or_si128(reg3_1, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}
							if ((i >> 1) & 0x1) {
								reg3_2 = _mm_or_si128(reg3_2, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}
							if ((i >> 2) & 0x1) {
								reg3_3 = _mm_or_si128(reg3_3, _mm_srli_epi16(fallPoint1, nowHeightArray[ts] + k));
							}

							//連鎖数をセット
							chain = specifiedChain(&reg3_1, &reg3_2, &reg3_3, i);

							if (chain > 0) {
								//聴牌している連鎖数を保持
								if (chain > chainMax_ready) {
									fireHeight = nowHeightArray[ts];
									chainMax_ready = chain;
								}

								//最大だったら入れ替える
								if (chainMax[value[range].first] < chain) {
									chainMax[value[range].first] = chain;
									first = value[range].first;
								}
								//1個で消えたら２個目は見なくていいのでkをインクリメント
								k++;
								break;
							}
						}
					}
					fallPoint1 = _mm_srli_si128(fallPoint1, 2); //自分の列の一番下(j+1, 1)にビットを移動

				}

				//****************評価値を計算(発火可能な連鎖数 + 窪みの少なさ)**************************
				evalVal = evaluation(nowHeightArray, chainMax_ready, fireHeight);

				//ここで14段目復活して格納(今回は復活無意味)
				valu.reg1 = _mm_or_si128(fourteen, reg2_1);
				valu.reg2 = _mm_or_si128(fourteen, reg2_2);
				valu.reg3 = _mm_or_si128(fourteen, reg2_3);

				//評価値を保存して配列に追加
				valu.value = evalVal;
				value.emplace_back(valu);
			}
		}

		//上位(RANGE)手を保存
		std::partial_sort(value.begin(), value.begin() + (count < RANGE ? count : RANGE), value.end(), std::greater<val>());
		value.erase(value.begin() + (count < RANGE ? count : RANGE), value.end());
	}
	//fieldChange(value[0].reg1, value[0].reg2, value[0].reg3, nowField);
	for (int i = 0; i < 22; i++) {
		pat[i] += chainMax[i];
	}
	return;
}

//評価値を計算(アプリ内でいじりたい)
int evaluation(volatile int nowHeightArray[6], int chainMax_ready, int fireHeight) {
	int evalVal; //戻り値、評価値
	int heightAve; //高さの平均
	int heightValue; //高さのバランス評価値格納

	heightAve = (nowHeightArray[0] + nowHeightArray[1] + nowHeightArray[2] + nowHeightArray[3] + nowHeightArray[4] + nowHeightArray[5]) / 6;
	heightValue = (heightAve + 2 - nowHeightArray[0]) * (heightAve + 2 - nowHeightArray[0]);
	heightValue += (heightAve - nowHeightArray[1]) * (heightAve - nowHeightArray[1]);
	heightValue += (heightAve - 1 - nowHeightArray[2]) * (heightAve - 1 - nowHeightArray[2]);
	heightValue += (heightAve - 1 - nowHeightArray[3]) * (heightAve - 1 - nowHeightArray[3]);
	heightValue += (heightAve - nowHeightArray[4]) * (heightAve - nowHeightArray[4]);
	heightValue += (heightAve + 2 - nowHeightArray[5]) * (heightAve + 2 - nowHeightArray[5]);

	//評価値実計算部分
	evalVal = -heightValue * 10;
	evalVal += chainMax_ready * (1000 * (heightAve + 1) / 10) + (100 * fireHeight);


	return evalVal;
}
