// puyopuyo.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"

#include <random>
#include <vector>
#include <iostream>
#include <immintrin.h>
#include <string>
#include <Windows.h>
#include <time.h>
#include <thread>
#include "Thinking.h"
#include "Resource.h"


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


// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
HWND hwnd_button_next; //次ボタン
HWND hwnd_button_forward; //前ボタン
HWND hwnd_button_reset;  //リセットボタン
HWND hwnd_button_change;  //変更ボタン
HWND hWnd;
HBITMAP hBitmap;
HDC     hdcMem;

RECT rx; //ウィンドウ領域
RECT cx; //クライアント領域

//ぷよぷよ用グローバル変数

std::vector<int> tsumo(128); //配ツモ128手
int turn = 0; //現在のターン
int nowField[8][16]; //今のフィールド状況
int controll[8192]; //置いた手を保管 => １手戻るよう

//次の手を決めるため
int pat[22];

//死亡判定
bool isDie = false;

volatile bool isFieldChange = false; //排他制御

//表示用
int nowChain;


//関数
void paint();
void makeTsumo(std::vector<int>& tsumo);
void fallTsumo(int field[8][16], std::vector<int> tsumo, int controll);
void fieldChange(__m128i reg1, __m128i reg2, __m128i reg3, int field[][16]);
bool chain(int field[][16], bool); //連鎖後のフィールドを返す
bool isChain(int field[][16]);
LPTSTR convertVal(int val); //数値を出力するために変換
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


	GetWindowRect(hWnd, &rx);
	GetClientRect(hWnd, &cx);
	const int new_width = 800 + (rx.right - rx.left) - (cx.right - cx.left);
	const int new_height = 600 + (rx.bottom - rx.top) - (cx.bottom - cx.top);
	SetWindowPos(hWnd, NULL, 0, 0, new_width, new_height, SWP_SHOWWINDOW);

	//bitmap関連
	GetClientRect(GetDesktopWindow(), &cx);  	// デスクトップのサイズを取得
	hBitmap = CreateCompatibleBitmap(GetDC(hWnd), cx.right, cx.bottom);
	hdcMem = CreateCompatibleDC(NULL);		// カレントスクリーン互換
	SelectObject(hdcMem, hBitmap);// MDCにビットマップを割り付け

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
			if (isFieldChange) break;
			//フィールドリセット
			isFieldChange = true;
			isDie = false;
			for (int i = 1; i < 7; i++) { //配列初期化
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

			isFieldChange = false;
			//再描画
			InvalidateRect(hWnd, NULL, false);

			break;
		}
		case WEIGHT: { // todo2 ) 重みを変えたい
			break;
		}
		case CHANGE: {
			if (isFieldChange) break;
			makeTsumo(tsumo);
			InvalidateRect(hWnd, NULL, false);
			break;
		}
		case NEXT: {
			if (isFieldChange) break;
			isFieldChange = true;

			int res = -1; //次の手のパターン（22手）
			int better = 0; //resの最大値取得用

			//	ツモの数(探索の深さを調整する)
			//	=>フィールドが埋まるほど浅い探索で良いから
			int deep = (6 * 13 - getAmount()) / 2 + 4;
			nowChain = 0;  //表示用

			//	思考　５個平行
			//	→大体画面が埋まるまで
			//	→ランダムなツモを用意して２２パターン試して評価値の上位raserパターン次の手を考える
			//	初手を保存しておいてそれに対応するpat[i]に最大の連鎖を保存する(一つのスレッドで一つの初手と連鎖数の組み合わせのみを返す)（同じ初手があれば合計の値を持つ）

			//	インスタンス生成
			Thinking think1, think2, think3, think4, think5, think6, think7;


			//	結果をリセット
			Thinking::resetPat();

			//	staticメンバ : フィールド情報をセット
			Thinking::setField(nowField);
			//	staticメンバ ; フィールド高さをセット
			Thinking::setHeight(Thinking::field_real[0], Thinking::fieldHeight_real);
			//	staticメンバ : ネクスト情報をセット
			Thinking::nextTsumo[0] = tsumo[(turn * 2) % 128];
			Thinking::nextTsumo[1] = tsumo[(turn * 2 + 1) % 128];
			Thinking::nextTsumo[2] = tsumo[(turn * 2 + 2) % 128];
			Thinking::nextTsumo[3] = tsumo[(turn * 2 + 3) % 128];
			Thinking::nextTsumo[4] = tsumo[(turn * 2 + 4) % 128];
			Thinking::nextTsumo[5] = tsumo[(turn * 2 + 5) % 128];
			//	staticメンバ : RANGE（ビームサーチの幅）をセット
			Thinking::RANGE = 400;

			//それぞれで最良の手を探索する
			std::thread t1([&think1, deep]() {think1.thinking(deep); });
			std::thread t2([&think2, deep]() {think2.thinking(deep); });
			std::thread t3([&think3, deep]() {think3.thinking(deep); });
			std::thread t4([&think4, deep]() {think4.thinking(deep); });
			std::thread t5([&think5, deep]() {think5.thinking(deep); });
			std::thread t6([&think6, deep]() {think6.thinking(deep); });
			std::thread t7([&think7, deep]() {think7.thinking(deep); });
			t1.join(), t2.join(), t3.join(), t4.join(), t5.join(), t6.join(), t7.join();

			//最良の手（一番連鎖数のおおかった初手を選んでツモを落とす）
			if (!Thinking::isDie) {
				for (int i = 0; i < 22; i++) {
					if (better < Thinking::pat[i]) {
						better = Thinking::pat[i];
						res = i;
					}
					Thinking::pat[i] = 0;
				}

				//操作手格納（戻せるように）
				controll[turn] = res;
				//ターン経過
				turn++;;

				//操作(フィールド変化)
				fallTsumo(nowField, tsumo, res);

				//連鎖確認
				if (isChain(nowField)) { //ちらつき防止用にチェックしてから再描画
					//つもを置いた手を描画する

					InvalidateRect(hWnd, NULL, false);
					UpdateWindow(hWnd); //強制再描画
					Sleep(600);
					std::thread chainPaint([hWnd]() {
						while (chain(nowField, true)) {
							nowChain++;
							//再描画
							InvalidateRect(hWnd, NULL, false);
							UpdateWindow(hWnd); //強制再描画
							std::this_thread::sleep_for(std::chrono::milliseconds(600));
						}
						isFieldChange = false;
						});
					chainPaint.detach();
				}
				else {
					//再描画
					InvalidateRect(hWnd, NULL, false);
					isFieldChange = false;
				}

			}
			else {
				isDie = true;
				//死にましたメッセージ?
			}

			


			break;
		}
		case DEBUG: {
			break;
		}
		case BACK: {
			isFieldChange = true;
			if (turn != 0) {
				isDie = true; //復活
				turn--;
				//操作手戻す
				controll[turn] = 0;
				//フィールド初期値に戻す
				for (int i = 1; i < 7; i++) {
					for (int j = 1; j < 15; j++) {
						nowField[i][j] = 0;
					}
				}

				//フィールド再構築(初手から順番においていくだけ)
				int tmpTurn = turn;
				for (turn = 1; turn <= tmpTurn; turn++) {
					fallTsumo(nowField, tsumo, controll[turn - 1]);
					chain(nowField, false);
				}
				turn = tmpTurn;
			}

			isFieldChange = false;

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
		//ビットマップを画面に転送
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		paint();
		BitBlt(hdc, 0, 0, cx.right, cx.bottom, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);

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







//**************:描画用******************************

//ツモを落とした後の形を取得（描画用）
void fallTsumo(int field[8][16], std::vector<int> tsumo, int controll) {
	turn--;
	switch (controll) {
	case 0: { //左2回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 0) % 128];
				field[1][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 1: { //左2回
		for (int i = 1; i < 14; i++) {
			if (field[1][i] == 0) {
				field[1][i] = tsumo[(turn * 2 + 1) % 128];
				field[1][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}

	case 2: {// Z←←
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

	case 3: {// X←
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


	case 4: { //左1回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 0) % 128];
				field[2][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 5: { //左1回
		for (int i = 1; i < 14; i++) {
			if (field[2][i] == 0) {
				field[2][i] = tsumo[(turn * 2 + 1) % 128];
				field[2][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}


	case 6: {// Z←
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

	case 7: {// X
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


	case 8: { //その場縦回転
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 0) % 128];
				field[3][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 9: { //その場
		for (int i = 1; i < 14; i++) {
			if (field[3][i] == 0) {
				field[3][i] = tsumo[(turn * 2 + 1) % 128];
				field[3][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}


	case 10: {// Z
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

	case 11: {// X→
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


	case 12: { //右1回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 0) % 128];
				field[4][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 13: { //右1回
		for (int i = 1; i < 14; i++) {
			if (field[4][i] == 0) {
				field[4][i] = tsumo[(turn * 2 + 1) % 128];
				field[4][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}


	case 14: {// Z→
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

	case 15: {// X→→
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

	case 16: { // 右2回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 0) % 128];
				field[5][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 17: { //右2回
		for (int i = 1; i < 14; i++) {
			if (field[5][i] == 0) {
				field[5][i] = tsumo[(turn * 2 + 1) % 128];
				field[5][i + 1] = tsumo[(turn * 2 + 0) % 128];
				break;
			}
		}
		break;
	}


	case 18: {// Z→→
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

	case 19: {// X→→→
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

	case 20: { //右3回縦回転
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 0) % 128];
				field[6][i + 1] = tsumo[(turn * 2 + 1) % 128];
				break;
			}
		}
		break;
	}

	case 21: { //右3回
		for (int i = 1; i < 14; i++) {
			if (field[6][i] == 0) {
				field[6][i] = tsumo[(turn * 2 + 1) % 128];
				field[6][i + 1] = tsumo[(turn * 2 + 0) % 128];
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


//フィールド描画
void paint() {
	static HBRUSH hOldBrush;
	static HBRUSH redBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0x00, 0x00));
	static HBRUSH greenBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0xFF, 0x7F));
	static HBRUSH blueBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0xBF, 0xFF));
	static HBRUSH yellowBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0xFF, 0x00));
	static HBRUSH purpleBrush = (HBRUSH)CreateSolidBrush(RGB(0x8A, 0x2B, 0xE2));
	static HBRUSH whiteBrush = (HBRUSH)CreateSolidBrush(RGB(0xFF, 0xFF, 0xFF));
	static HBRUSH blackBrush = (HBRUSH)CreateSolidBrush(RGB(0x00, 0x00, 0x00));
	static HBRUSH blownBrush = (HBRUSH)CreateSolidBrush(RGB(0x86, 0x4A, 0x2B));
	hOldBrush = (HBRUSH)SelectObject(hdcMem, whiteBrush);
	FillRect(hdcMem, &cx, whiteBrush); //背景を白く塗りつぶす
	//連鎖数描画
	Rectangle(hdcMem, 300, 410, 460, 450);
	static LPCWSTR chainStr = L"連鎖数";
	TextOut(hdcMem, 350, 400, chainStr, lstrlen(chainStr));
	LPCWSTR nowChainStr = convertVal(nowChain);
	TextOut(hdcMem, 350, 420, nowChainStr, lstrlen(nowChainStr));

	//フィールド描画
	for (int i = 13; i >= 1; i--) {
		for (int j = 0; j < 8; j++) {

			switch (nowField[j][i]) {
			case 1: hOldBrush = (HBRUSH)SelectObject(hdcMem, redBrush);  break;
			case 2: hOldBrush = (HBRUSH)SelectObject(hdcMem, greenBrush);  break;
			case 3: hOldBrush = (HBRUSH)SelectObject(hdcMem, blueBrush);  break;
			case 4: hOldBrush = (HBRUSH)SelectObject(hdcMem, yellowBrush); break;
			case 7: hOldBrush = (HBRUSH)SelectObject(hdcMem, blackBrush); break;
			default: hOldBrush = (HBRUSH)SelectObject(hdcMem, whiteBrush);
			}

			Rectangle(hdcMem, FIELD_LEFT + (j * 20), FIELD_TOP + ((14 - i) * 20), FIELD_LEFT + (j * 20) + TSUMO_SIZE, FIELD_TOP + ((14 - i) * 20) + TSUMO_SIZE);
		}
	}

	//壁描画
	hOldBrush = (HBRUSH)SelectObject(hdcMem, blownBrush);
	for (int j = 0; j < 8; j += 7) {
		for (int i = 12; i >= 0; i--) {

			Rectangle(hdcMem, FIELD_LEFT + (j * 20), FIELD_TOP + ((14 - i) * 20), FIELD_LEFT + (j * 20) + TSUMO_SIZE, FIELD_TOP + ((14 - i) * 20) + TSUMO_SIZE);
		}
	}
	for (int j = 0; j < 8; j++) {
		int i = 0;
		Rectangle(hdcMem, FIELD_LEFT + (j * 20), FIELD_TOP + ((14 - i) * 20), FIELD_LEFT + (j * 20) + TSUMO_SIZE, FIELD_TOP + ((14 - i) * 20) + TSUMO_SIZE);
	}

	//ツモ描画
	for (int i = 0; i < 2; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdcMem, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdcMem, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdcMem, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdcMem, yellowBrush); break;
		}
		Rectangle(hdcMem, FIELD_LEFT + 60, FIELD_TOP - 40 + (i * 20), FIELD_LEFT + 60 + TSUMO_SIZE, FIELD_TOP - 40 + (i * 20) + TSUMO_SIZE);
	}
	for (int i = 2; i < 4; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdcMem, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdcMem, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdcMem, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdcMem, yellowBrush); break;
		}
		Rectangle(hdcMem, FIELD_LEFT + 220, FIELD_TOP + (i * 20), FIELD_LEFT + 220 + TSUMO_SIZE, FIELD_TOP + (i * 20) + TSUMO_SIZE);
	}
	for (int i = 4; i < 6; i++) {
		switch (tsumo[(i + turn * 2) % 128]) {
		case 1: hOldBrush = (HBRUSH)SelectObject(hdcMem, redBrush); break;
		case 2: hOldBrush = (HBRUSH)SelectObject(hdcMem, greenBrush); break;
		case 3: hOldBrush = (HBRUSH)SelectObject(hdcMem, blueBrush); break;
		case 4: hOldBrush = (HBRUSH)SelectObject(hdcMem, yellowBrush); break;
		}
		Rectangle(hdcMem, FIELD_LEFT + 220, FIELD_TOP + 20 + (i * 20), FIELD_LEFT + 220 + TSUMO_SIZE, FIELD_TOP + 20 + (i * 20) + TSUMO_SIZE);
	}
	//DeleteObject(SelectObject(hdcMem, hOldBrush));
	//EndPaint(hWnd, &ps);
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

//レジスタから配列に格納(chainで使う)
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


//連鎖があるかを確認する
bool isChain(int field[][16]) {

	//配列からビットに変換
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

	unsigned int mask[] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000 }; //列のツモ数に応じた、全ツモが下に落ちたときの位置

	__m128i field1 = _mm_set_epi64x(llA, llA2);
	__m128i field2 = _mm_set_epi64x(llB, llB2);
	__m128i field3 = _mm_set_epi64x(llC, llC2);

	//__m128i ojama = _mm_and_si128(reg1, _mm_and_si128(reg2, reg3));
	__m128i colorReg;
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
	__m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13段目にビットが立っている
	__m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);

	uint16_t* cnt = (uint16_t*)&vanish;
	uint64_t* pdepMaskPtr = (uint64_t*)&pdepMask;
	uint64_t* pextMaskPtr = (uint64_t*)&pextMask;
	uint64_t* fieldPtr1 = (uint64_t*)&field1;
	uint64_t* fieldPtr2 = (uint64_t*)&field2;
	uint64_t* fieldPtr3 = (uint64_t*)&field3;
	uint64_t* vanishPtr = (uint64_t*)&vanish;


	//連鎖数を返す
	int count = 0;
	while (true) {
		//13段目の情報を保存
		//その後、フィールド(1～12段目)以外の情報を消す
		thirteen[0] = _mm_and_si128(vanish13, field1);
		thirteen[1] = _mm_and_si128(vanish13, field2);
		thirteen[2] = _mm_and_si128(vanish13, field3);
		field1 = _mm_and_si128(vanishOther, field1);
		field2 = _mm_and_si128(vanishOther, field2);
		field3 = _mm_and_si128(vanishOther, field3);

		for (int color = 1; color < 5; color++) {
			switch (color) {
			case 1: colorReg = _mm_andnot_si128(field3, _mm_andnot_si128(field2, field1)); break;//赤
			case 2: colorReg = _mm_andnot_si128(field3, _mm_andnot_si128(field1, field2)); break;//緑
			case 3: colorReg = _mm_andnot_si128(field3, _mm_and_si128(field1, field2)); break;//青
			case 4: colorReg = _mm_andnot_si128(field2, _mm_andnot_si128(field1, field3)); break;//黄色
			case 5: colorReg = _mm_andnot_si128(field2, _mm_and_si128(field1, field3)); break; //紫
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
		}
		//vanishが0なら抜けたい

		if ((vanishPtr[0] | vanishPtr[1]) == 0) {
			return false;
		}
		return true;
	}
}

//連鎖表示用
bool chain(int field[][16], bool isPaint)
{

	//配列からビットに変換
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

	unsigned int mask[] = { 0xffff, 0xfffe, 0xfffc, 0xfff8, 0xfff0, 0xffe0, 0xffc0, 0xff80, 0xff00, 0xfe00, 0xfc00, 0xf800, 0xf000, 0xe000 }; //列のツモ数に応じた、全ツモが下に落ちたときの位置

	__m128i field1 = _mm_set_epi64x(llA, llA2);
	__m128i field2 = _mm_set_epi64x(llB, llB2);
	__m128i field3 = _mm_set_epi64x(llC, llC2);

	//__m128i ojama = _mm_and_si128(reg1, _mm_and_si128(reg2, reg3));
	__m128i colorReg;
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
	__m128i vanish13 = _mm_set_epi64x(0x0000000400040004, 0x0004000400040000); //13段目にビットが立っている
	__m128i vanishOther = _mm_set_epi64x(0x00007ff87ff87ff8, 0x7ff87ff87ff80000);

	uint16_t* cnt = (uint16_t*)&vanish;
	uint64_t* pdepMaskPtr = (uint64_t*)&pdepMask;
	uint64_t* pextMaskPtr = (uint64_t*)&pextMask;
	uint64_t* fieldPtr1 = (uint64_t*)&field1;
	uint64_t* fieldPtr2 = (uint64_t*)&field2;
	uint64_t* fieldPtr3 = (uint64_t*)&field3;
	uint64_t* vanishPtr = (uint64_t*)&vanish;


	//連鎖数を返す
	int count = 0;
	while (true) {
		//13段目の情報を保存
		//その後、フィールド(1～12段目)以外の情報を消す
		thirteen[0] = _mm_and_si128(vanish13, field1);
		thirteen[1] = _mm_and_si128(vanish13, field2);
		thirteen[2] = _mm_and_si128(vanish13, field3);
		field1 = _mm_and_si128(vanishOther, field1);
		field2 = _mm_and_si128(vanishOther, field2);
		field3 = _mm_and_si128(vanishOther, field3);

		for (int color = 1; color < 5; color++) {
			switch (color) {
			case 1: colorReg = _mm_andnot_si128(field3, _mm_andnot_si128(field2, field1)); break;//赤
			case 2: colorReg = _mm_andnot_si128(field3, _mm_andnot_si128(field1, field2)); break;//緑
			case 3: colorReg = _mm_andnot_si128(field3, _mm_and_si128(field1, field2)); break;//青
			case 4: colorReg = _mm_andnot_si128(field2, _mm_andnot_si128(field1, field3)); break;//黄色
			case 5: colorReg = _mm_andnot_si128(field2, _mm_and_si128(field1, field3)); break; //紫
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
		}
		//vanishが0なら抜けたい

		if ((vanishPtr[0] | vanishPtr[1]) == 0) {
			if (!isPaint) { //1手戻るで復元中なら
				//13段目復活
				field1 = _mm_or_si128(thirteen[0], field1);
				field2 = _mm_or_si128(thirteen[1], field2);
				field3 = _mm_or_si128(thirteen[2], field3);
			}
			else { //描画中なら何も変えずに返す
				field1 = _mm_set_epi64x(llA, llA2);
				field2 = _mm_set_epi64x(llB, llB2);
				field3 = _mm_set_epi64x(llC, llC2);
			}
			break;
		}
		count++;
		// ここで上下左右におじゃまがあったらフラグ立てればいい
		//tmpRegV = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_epi16(vanish, 1)), _mm_and_si128(ojama, _mm_srli_epi16(vanish, 1)));
		//tmpRegH = _mm_or_si128(_mm_and_si128(ojama, _mm_slli_si128(vanish, 2)), _mm_and_si128(ojama, _mm_srli_si128(vanish, 2)));
		//vanish = _mm_or_si128(vanish, _mm_or_si128(tmpRegV, tmpRegH));
		//unsigned long a = mask[__popcnt16(cnt[5])];

		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));
		field1 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field2 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field3 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		//ここで13段目復活
		field1 = _mm_or_si128(thirteen[0], field1);
		field2 = _mm_or_si128(thirteen[1], field2);
		field3 = _mm_or_si128(thirteen[2], field3);
		vanish = _mm_and_si128(_mm_or_si128(vanishOther, vanish13), _mm_xor_si128(_mm_or_si128(field3, _mm_or_si128(field2, field1)), _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff)));
		pdepMask = _mm_setr_epi16(mask[__popcnt16(cnt[0])], mask[__popcnt16(cnt[1])], mask[__popcnt16(cnt[2])], mask[__popcnt16(cnt[3])], mask[__popcnt16(cnt[4])], mask[__popcnt16(cnt[5])], mask[__popcnt16(cnt[6])], mask[__popcnt16(cnt[7])]);
		pextMask = _mm_xor_si128(vanish, _mm_set_epi64x(0xffffffffffffffff, 0xffffffffffffffff));

		field1 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr1[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr1[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field2 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr2[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr2[0], pextMaskPtr[0]), pdepMaskPtr[0]));
		field3 = _mm_set_epi64x(_pdep_u64(_pext_u64(fieldPtr3[1], pextMaskPtr[1]), pdepMaskPtr[1]), _pdep_u64(_pext_u64(fieldPtr3[0], pextMaskPtr[0]), pdepMaskPtr[0]));

		vanish = _mm_setzero_si128();
		if (isPaint) {
			fieldChange(field1, field2, field3, field);
			return true;
		}
	}

	fieldChange(field1, field2, field3, field);
	return false;

}


//その他関数


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

//128手均等に色を作る
void makeTsumo(std::vector<int>& tsumo) {
	for (int i = 0; i < 128; i++) {
		tsumo[i] = i / 32 + 1;
	}
	std::random_device get_rand_dev;
	std::mt19937 get_rand_mt(get_rand_dev()); // seedに乱数を指定
	std::shuffle(tsumo.begin(), tsumo.end(), get_rand_mt);
}

