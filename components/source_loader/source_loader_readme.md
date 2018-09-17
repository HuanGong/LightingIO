# source loader

```json
{
    "reader": {
      "type": "file",
      "mode": "line",
      "resources": [
        {
          "path": "./data_file_test.txt"
        }
      ]
    }
}
```
- type: 资源加载器的类型，用于工厂函数创建对应的资源加载器
    - file  文件模式读取
    - mysql mysql加载器
    - http  http网络加载器
  
- mode: 资源的解析模式，意味着数据读取之后，以何种形式传递给parser
    - line 按行读取，每读取一行，通过委托的形式传递给 parser
    - content 整个文件内容一次性传递给parse解析

- resource: 资源对象数组
    - 对于文件加载器，里面为文件路径path
    - 对于http加载器， 里面为url path
    - 对于mysql加载器， 里面为mysql信息以及query语句
