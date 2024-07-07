# Sample

## 概要
このプロジェクトはランタイムで3Dモデルを読み込み、3D空間上に表示することを目的としています。  
※現在は、静的に指定された2Dテクスチャを表示する機能までです。  

## 要点
### ダブルバッファリング
描画用のバッファーと表示用のバッファーを生成します。  
`swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i]));`  

出力処理の最後に二つのバッファーを入れ替えます  
`swapChain->Present(1, 0);`  
上記の処理によって、ティアリングを防ぐことができます。  

### グラフィックスパイプライン
以下のステージを経て、出力します。　  
※2024/7月時点  
**IA -> VS -> RS -> PS -> OM**  

### フェンス
CPUとGPUの処理を同期させるために、フェンスを導入します。  
`device->CreateFence(fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence);`  
`cmdQueue->Signal(fence, ++fenceVal);`  
`fence->SetEventOnCompletion(fenceVal, event);`  

### バリア
リソースの状態遷移を記述します。  
```
BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
```


 ### ビューポート
 レンダリング結果の表示形式を指定します。  
 ```
 viewport.Width = (FLOAT)windowWidth;  
 viewport.Height = (FLOAT)windowHeight;
 viewport.TopLeftX = 0;
 viewport.TopLeftY = 0;
 viewport.MaxDepth = 1.0f;
 viewport.MinDepth = 0.0f;
 ```
 

 ### シザー矩形
 ビューポートに出力された画像の表示範囲を指定します。
 ```
 scissorrect.top = 0;
 scissorrect.left = 0;
 scissorrect.right = scissorrect.left + windowWidth;
 scissorrect.bottom = scissorrect.right + windowHeight;
 ``` 
 

 ### 頂点レイアウト
 以下の形式で頂点レイアウトを指定します。 ※2024/7月時点  
 ```
 struct BasicType
 {
   float4 svpos : SV_POSITION; //頂点座標
   float2 uv : TEXCOORD; //UV値
 };
 ```
 

## 参考文献
・DirectX12の魔導書　3Dレンダリングの基礎からMMDモデルを踊らせるまで  
https://www.shoeisha.co.jp/book/detail/9784798161938

・ゲームプログラミングC++  
https://www.shoeisha.co.jp/book/detail/9784798157610
