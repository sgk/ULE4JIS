# ULE4JIS アーキテクチャとビルドノート

このドキュメントは、現在のコードベースの動作と、Windows上で変更・再ビルドする前に知っておくべき実用的なポイントをまとめたものです。

## 1. プロジェクトタイプとツールチェーン

- Visual Studio 2005 形式の古い MFC デスクトップアプリケーション
- ソリューション/プロジェクトファイル：
  - `Ule4Jis.sln` (Format Version 9.00, VS2005)
  - `Ule4Jis/Ule4Jis.vcproj` (Visual C++ project, `Version="8.00"`)
- ビルド構成は `Debug|Win32` と `Release|Win32` のみ
- MFC が有効（`UseOfMFC="1"`）
- アプリは `boost::ptr_map` と `std::auto_ptr` を使用（現在は独自の `PtrMap` を使用）

最新の Visual Studio での影響：

- 最近の Visual Studio で開くと、`.vcproj` からのプロジェクトアップグレードが必要
- 最新の言語標準でビルドする場合、`std::auto_ptr` を `std::unique_ptr` に移行する必要がある可能性

## 2. アプリの機能

- ULE4JIS は、タスクトレイ常駐型のキーボードレイアウトエミュレーター
- OS が JIS キーボードドライバを使用している状態で、US キーボードの動作をエミュレート
- キーボードドライバはインストールせず、低レベルキーボードフックと合成キーイベントを使用

主なユーザー向けの動作：

- ダイアログアプリとして起動するが、タスクトレイでの使用を想定
- 起動時にタスクトレイアイコンを追加
- 起動直後からキーボードエミュレーションを開始
- トレイメニューで開始/停止/終了をサポート

## 3. 実行フロー（概要）

1. `Ule4JisApp::InitInstance()` がシングルインスタンスミューテックスを作成し、メインダイアログを開く
2. `Ule4JisDlg::OnInitDialog()`:
   - トレイアイコンを初期化
   - `USonJISStrategy` で `KeyEmulator` を作成
   - `start()` を呼び出してフックを有効化
3. `KeyHooker` が `WH_KEYBOARD_LL` をインストール
4. 各キーボードイベントに対して：
   - フックコールバックが `KeyHookEventArgs` を構築
   - `KeyEmulator::onKeyHookEvent()` に転送
5. `KeyEmulator` が `KeyCondition` を更新し、マップからエミュレーションコマンドを検索：
   - マップされたエミュレーションが見つかれば実行
   - `true` を返して元のキーイベントをキャンセル
   - 見つからなければスルー
6. 合成キーイベントは `keybd_event()` で `dwExtraInfo=(this)` を使用して送信し、自分が発したイベントを再処理しないようにする

## 4. コアファイルと責務

- アプリ / UI / トレイ
  - `Ule4Jis/Ule4Jis.cpp`
  - `Ule4Jis/Ule4JisDlg.h`
  - `Ule4Jis/Ule4JisDlg.cpp`
  - `Ule4Jis/Ule4Jis.rc`
  - `Ule4Jis/resource.h`

- フックとイベント伝達
  - `Ule4Jis/KeyHooker.h`
  - `Ule4Jis/KeyHooker.cpp`
  - `Ule4Jis/KeyHookEventArgs.h`
  - `Ule4Jis/KeyHookEventArgs.cpp`
  - `Ule4Jis/KeyHookEventListener.h`

- エミュレーションエンジン
  - `Ule4Jis/KeyEmulator.h`
  - `Ule4Jis/KeyEmulator.cpp`
  - `Ule4Jis/KeyCondition.h`
  - `Ule4Jis/KeyCondition.cpp`
  - `Ule4Jis/Emulation.h`
  - `Ule4Jis/EmulationStrategy.h`

- エミュレーションコマンド実装
  - `Ule4Jis/NormalKeyEmulation.h/.cpp`
  - `Ule4Jis/NopEmulation.h`
  - `Ule4Jis/PressAndReleaseDecorator.h/.cpp`
  - `Ule4Jis/ShiftPressDecorator.h/.cpp`
  - `Ule4Jis/ShiftReleaseDecorator.h/.cpp`

- マッピング戦略
  - `Ule4Jis/USonJISStrategy.h`
  - `Ule4Jis/USonJISStrategy.cpp`
  - `Ule4Jis/NopStrategy.h`

## 5. キーマッピングの表現方法

- マッピングテーブルの型: `PtrMap<KeyCondition, Emulation>` (旧: `boost::ptr_map`)
- `KeyCondition` キーの実質的な内容：
  - 最後のキー (`lastVKey`)
  - Shift 状態（左右を統合）
  - Alt 状態（左右を統合）
- 注意: Control 状態は追跡されるが、比較からは意図的に除外される

意味：

- マッピング検索は、現在のキー + Shift/Alt コンテキストで行われる
- マッピングエントリが存在しない場合、入力は変更されない

## 6. 現在のアクティブな戦略とマップされたキー

- `USonJISStrategy` が唯一のアクティブな戦略
- 記号キーの再マッピングロジックをハードコード（例: `@`, `^`, `&`, `*`, `(`, `)`, `_`, `=`, `+`, `` ` ``, `~`, `[`, `]`, `{`, `}`, `:`, `'`, `"`, `\`, `|`）
- 一部のマッピングは、Shift を一時的に押す/離す、またはキー押下+解放を強制するためにデコレーターを使用

## 7. トレイ制御の動作

### トレイメニューのアクション

- **有効化（Start/Enable）**:
  - `keyEmulator->start()` を呼び出し
  - トレイアイコンが US アイコンに切り替わる
  - Caps Lock モードが無効でなく、Caps Lock LED が点灯している場合、自動的にオフにする
- **無効化（Stop/Disable）**:
  - `keyEmulator->end()` を呼び出し
  - トレイアイコンが JIS アイコンに切り替わる
- **終了（Exit）**:
  - 終了メッセージを送信
  - 設定はレジストリに保存される

### Caps Lock の動作（3モード）

- **Alt + ` (AltBackquote)**: スキャンコード 0x29（全角/半角キー）を使用する従来の方式
  - 合成キーイベントを送信して IME を切り替え（日本語キーボード向け）
- **IMEを直接オンオフ (DirectIME)**: Windows IME API 経由で直接 IME を制御
  - `ImmGetDefaultIMEWnd()` と `SendMessage(WM_IME_CONTROL, IMC_SETOPENSTATUS)` を使用
  - スキャンコード方式が動作しない US キーボードでより確実
- **無効 (Disabled)**: Caps Lock キーが通常の Caps Lock として動作

### ウィンドウの動作

- 最小化するとメインウィンドウが非表示になり、トレイ常駐を維持

### 設定の永続化

- 設定は `HKEY_CURRENT_USER\Software\ULE4JIS` に保存される：
  - `Enabled` (DWORD): エミュレーション有効状態（1=有効, 0=無効）
  - `CapsLockMode` (DWORD): Caps Lock モード（0=AltBackquote, 1=DirectIME, 2=Disabled）
  - `USonJIS` (DWORD): US on JIS 戦略の有効状態（1=有効, 0=無効）

## 8. 変更前の重要な注意点

- `KeyHooker` は静的シングルトンのようなメンバー（`hookHandle`, `thisPtr`）を使用するため、アクティブなフッカーインスタンスは1つのみを想定
- フックのインストール失敗時は `new CUserException()` をスロー（ヒープ割り当てスタイルは古く、例外処理を近代化する場合は見直しが必要）
- `.vcproj` のコンパイルリストに含まれていないソースファイルが少なくとも1つ存在する（`KeyHookEventListener.cpp`）ため、`.vcproj` ファイルリストをビルドの真実として扱うべき
- `stdafx.h` のプラットフォームマクロは XP 時代の値をターゲットとしている（`WINVER/_WIN32_WINNT = 0x0501`）

## 9. 変更タイプ別の推奨「最初に触るべき」ファイル

### キーマッピングの動作を変更
- `Ule4Jis/USonJISStrategy.cpp`
- `Ule4Jis/KeyCondition.cpp`（条件のキー化ルールを変更する必要がある場合）

### トレイ/UI の動作を変更
- `Ule4Jis/Ule4JisDlg.cpp`
- `Ule4Jis/Ule4Jis.rc`
- `Ule4Jis/resource.h`

### ライフサイクル/起動の動作を変更
- `Ule4Jis/Ule4Jis.cpp`
- `Ule4Jis/Ule4JisDlg.cpp`

## 10. クイック再ビルドチェックリスト（Windows）

1. Visual Studio で `Ule4Jis.sln` を開く
2. プロンプトが表示されたら、VS2005 形式からのプロジェクトアップグレードを許可
3. MFC ワークロード/コンポーネントがインストールされていることを確認
4. まず `Debug|Win32` をビルドし、次に `Release|Win32` をビルド
5. 最新のコンパイラが `auto_ptr` でエラーを出す場合、続行前にポインタ型を移行

## 11. ビルド環境（最新）

### 必要な環境
- **Visual Studio 2022 以降**
- **Platform Toolset:** v143 以降
- **Windows SDK:** 10.0
- **C++ 標準:** C++14 (stdcpp14)

### 必要なコンポーネント
Visual Studio Installer で以下をインストール：
- **C++によるデスクトップ開発**
- **MFC および ATL のサポート（x86 および x64）**
- **Windows 10/11 SDK**

### 生成ファイル
- **デバッグ:** `Debug\Ule4Jis.exe`
- **リリース:** `Release\Ule4Jis.exe`
