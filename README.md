## Example

```cpp
#include "differ.h"

const auto operations = diff("tree", "free");
for (const auto &operation : operations) {
    if (auto insertOperation = std::get_if<InsertOperation>(&operation)) {
        qDebug() << "insert" << dst[insertOperation->offset] << "at" << insertOperation->index;
    } else if (auto removeOperation = std::get_if<RemoveOperation>(&operation)) {
        qDebug() << "remove" << removeOperation->count << "items at" << removeOperation->offset;
    }
}
```

The code snippet above will produce the following output

```
remove 1 items at 0
insert 'f' at 0
```
