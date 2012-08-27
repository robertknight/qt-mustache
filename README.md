# Qt Mustache

qt-mustache is a simple library for rendering [Mustache templates](http://mustache.github.com/).

### Example Usage

```cpp
#include "mustache.h"

QVariantMap contact;
contact["name"] = "John Smith";
contact["email"] = "john.smith@gmail.com";

QString contactTemplate = "<b>{{name}}</b> <a href=\"mailto:{{email}}\">{{email}}</a>";

Mustache::Renderer renderer;
Mustache::QtVariantContext context(contact);

QTextStream output(stdout);
output << renderer.render(contactTemplate, &context);
```

Outputs: '<b>John Smith</b> <a href="mailto:john.smith@gmail.com">john.smith@gmail.com</a>'

For further examples, see the tests in `test_mustache.cpp`

### Building
 * To build the tests, run `qmake` followed by `make`
 * To use qt-mustache in your project, just add the `mustache.h` and `mustache.cpp` files to your project.
  
### License
 qt-mustache is licensed under the BSD license. 

### Dependencies
 qt-mustache depends on the QtCore and QtGui libraries from Qt 4.  The QtGui dependency can be easily removed if necessary.
 