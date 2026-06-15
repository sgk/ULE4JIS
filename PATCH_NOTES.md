
# 変更履歴

## Boost依存の削除

- `boost::ptr_map` の代替として、最小限の `PtrMap.h` を追加
- `EmulationStrategy.h` の typedef を `boost::ptr_map` から `PtrMap` に変更
- `stdafx.h` から Boost のインクルードを削除し、`PtrMap.h` を追加
- `stdafx.h` に `<memory>` をインクルードして `std::auto_ptr` を定義

## Caps Lock 機能の拡張（2026年1月）

- Caps Lock キーの動作を単純なトグルから3つのモードに拡張：
  - **Alt + ` (AltBackquote)**: 従来のスキャンコード方式（日本語キーボード向け）
  - **IMEを直接オンオフ (DirectIME)**: IME API を直接制御（USキーボード向け）
  - **無効 (Disabled)**: 通常の Caps Lock として動作
- 起動時に Caps Lock LED が点灯している場合、自動的にオフにする機能を追加（無効モードを除く）
- `ImmGetDefaultIMEWnd()` と `SendMessage()` を使用した IME 制御を実装し、USキーボードとの互換性を向上
- タスクトレイメニューに3つの Caps Lock オプションを表示（チェックマーク付き）
- 設定をレジストリに保存（`HKEY_CURRENT_USER\Software\ULE4JIS\CapsLockMode`）

## メニュー表示の改善

- Caps Lock オプションの表示方法を試行錯誤し、最終的に標準のチェックマーク（`MF_CHECKED`）を採用
- インデントなしで項目を表示し、選択されている項目のみチェックマークを左側に表示
- すべてのテキストを左揃えにして、視認性を向上
