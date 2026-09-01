# 批量处理

RPGMTranslate 允许对翻译内容轻松应用一些批量处理。通过 ![](./assets/construction.svg) 按钮访问。

使用文件选择菜单选择要运行的文件，然后选择一个操作：

## 翻译

使用你配置的[翻译服务](./settings.md#translation)对所选文件中的源文本运行机器翻译，将结果写入所选的译文列。

## 脚本

对所选文件中的文本运行脚本，而非翻译。内置了三个脚本：

- **Trim（去首尾空白）**——去除文本首尾的空白字符。
- **Wrap（自动换行）**——将文本按指定字符长度换行。"使用设置中的值"按钮会从项目的行长度提示中填充此值（请参阅[项目设置](./settings.md#project)。
- **Latinize（拉丁化）**——将文本音译为拉丁字符。

## 自定义脚本

你也可以编写自己的脚本，使用 JavaScript（由 [quickjs-ng](https://github.com/quickjs-ng/quickjs) 提供支持）。自定义脚本会保存到应用程序同目录下的 `scripts` 文件夹中，因此可以在不同会话之间保持，并且易于分享。

脚本必须定义一个函数：

```js
function rpgmLineCallback(
  sourceText,
  translations,
  filename,
  lineNumber,
  options,
) {
  // ...
  return translations;
}
```

它会对每个可翻译行调用一次：

- `sourceText`——该行的源文本。
- `translations`——包含该行现有译文列的数组。
- `filename`——该行所属的文件。
- `lineNumber`——该行在该文件中的索引。
- `options`——`{ action, columnIndex, ... }`；`action` 为 `"trim"`/`"wrap"`/`"latinize"`/`"custom"`，`columnIndex` 是正在写入的译文列，而 `trimLeading`/`trimTrailing`（用于 trim）或 `wrapLength`（用于 wrap）携带内置脚本的设置。

返回一个字符串数组以覆盖译文列——数组元素 `i` 对应 `translations[i]`。为某个元素返回 `undefined`/`null`（或返回一个较短的数组）将使该列保持不变。
