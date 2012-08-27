/*
  Copyright 2012, Robert Knight

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
*/

#include "test_mustache.h"

void TestMustache::testValues()
{
	QVariantHash map;
	map["name"] = "John Smith";
	map["age"] = 42;
	map["sex"] = "Male";
	map["company"] = "Smith & Co";
	map["signature"] = "John Smith of <b>Smith & Co</b>";

	QString _template = "Name: {{name}}, Age: {{age}}, Sex: {{sex}}\n"
	                    "Company: {{company}}\n"
						"  {{{signature}}}"
	                    "{{missing-key}}";
	QString expectedOutput = "Name: John Smith, Age: 42, Sex: Male\n"
	                         "Company: Smith &amp; Co\n"
	                         "  John Smith of <b>Smith & Co</b>";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);
}

QVariantHash contactInfo(const QString& name, const QString& email)
{
	QVariantHash map;
	map["name"] = name;
	map["email"] = email;
	return map;
}

void TestMustache::testSections()
{
	QVariantHash map = contactInfo("John Smith", "john.smith@gmail.com");
	QVariantList contacts;
	contacts << contactInfo("James Dee", "james@dee.org");
	contacts << contactInfo("Jim Jones", "jim-jones@yahoo.com");
	map["contacts"] = contacts;

	QString _template = "Name: {{name}}, Email: {{email}}\n"
	                    "{{#contacts}}  {{name}} - {{email}}\n{{/contacts}}"
						"{{^contacts}}  No contacts{{/contacts}}";

	QString expectedOutput = "Name: John Smith, Email: john.smith@gmail.com\n"
	                         "  James Dee - james@dee.org\n"
	                         "  Jim Jones - jim-jones@yahoo.com\n";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);

	// test inverted sections
	map.remove("contacts");
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);

	expectedOutput = "Name: John Smith, Email: john.smith@gmail.com\n"
	                 "  No contacts";
	QCOMPARE(output, expectedOutput);

	// test with an empty list instead of an empty key
	map["contacts"] = QVariantHash();
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);
	QCOMPARE(output, expectedOutput);
}

void TestMustache::testContextLookup()
{
	QVariantHash fileMap;
	fileMap["dir"] = "/home/robert";
	fileMap["name"] = "robert";

	QVariantList files;
	QVariantHash file;
	file["name"] = "test.pdf";
	files << file;

	fileMap["files"] = files;

	QString _template = "{{#files}}{{dir}}/{{name}}{{/files}}";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(fileMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("/home/robert/test.pdf"));
}

void TestMustache::testPartials()
{
	QHash<QString, QString> partials;
	partials["file-info"] = "{{name}} {{size}} {{type}}\n";

	QString _template = "{{#files}}{{>file-info}}{{/files}}";

	QVariantHash map;
	QVariantList fileList;
	
	QVariantHash file1;
	file1["name"] = "mustache.pdf";
	file1["size"] = "200KB";
	file1["type"] = "PDF Document";

	QVariantHash file2;
	file2["name"] = "cv.doc";
	file2["size"] = "300KB";
	file2["type"] = "Microsoft Word Document";

	fileList << file1 << file2;
	map["files"] = fileList;

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output,
	 QString("mustache.pdf 200KB PDF Document\n"
	         "cv.doc 300KB Microsoft Word Document\n"));
}

void TestMustache::testSetDelimiters()
{
	QVariantHash map;
	map["name"] = "John Smith";
	map["phone"] = "01234 567890";

	QString _template =
	  "{{=<% %>=}}"
	  "<%name%>{{ }}<%phone%>"
	  "<%={{ }}=%>"
	  " {{name}}<% %>{{phone}}";

	QString expectedOutput = "John Smith{{ }}01234 567890 John Smith<% %>01234 567890";
	
	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);
}

void TestMustache::testErrors()
{
	QVariantHash map;
	map["name"] = "Jim Jones";

	QHash<QString, QString> partials;
	partials["buggy-partial"] = "--{{/one}}--";

	QString _template = "{{name}}";

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("Jim Jones"));
	QCOMPARE(renderer.error(), QString());
	QCOMPARE(renderer.errorPos(), -1);

	_template = "{{#one}} {{/two}}";
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), QString("Tag start/end key mismatch"));
	QCOMPARE(renderer.errorPos(), 9);
	QCOMPARE(renderer.errorPartial(), QString());

	_template = "Hello {{>buggy-partial}}";
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), QString("Unexpected end tag"));
	QCOMPARE(renderer.errorPos(), 2);
	QCOMPARE(renderer.errorPartial(), QString("buggy-partial"));
}

void TestMustache::testPartialFile()
{
	QString path = QCoreApplication::applicationDirPath();

	QVariantHash map = contactInfo("Jim Smith", "jim.smith@gmail.com");

	QString _template = "{{>partial}}";

	Mustache::Renderer renderer;
	Mustache::PartialFileLoader partialLoader(path);
	Mustache::QtVariantContext context(map, &partialLoader);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("Jim Smith -- jim.smith@gmail.com\n"));
}

void TestMustache::testEscaping()
{
	QVariantHash map;
	map["escape"] = "<b>foo</b>";
	map["unescape"] = "One &amp; Two";
	map["raw"] = "<b>foo</b>";

	QString _template = "{{escape}} {{&unescape}} {{{raw}}}";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("&lt;b&gt;foo&lt;/b&gt; One & Two <b>foo</b>"));
}

class CounterContext : public Mustache::QtVariantContext
{
public:
	int counter;

	CounterContext(const QVariantHash& map)
	: Mustache::QtVariantContext(map)
	, counter(0)
	{}

	virtual bool canEval(const QString& key) const {
		return key == "counter";
	}

	virtual QString eval(const QString& key, const QString& _template, Mustache::Renderer* renderer) {
		if (key == "counter") {
			++counter;
		}
		return renderer->render(_template, this);
	}

	virtual QString stringValue(const QString& key) const {
		if (key == "count") {
			return QString::number(counter);
		} else {
			return Mustache::QtVariantContext::stringValue(key);
		}
	}
};

void TestMustache::testEval()
{
	QVariantHash map;
	QVariantList list;
	list << contactInfo("Rob Knight", "robertknight@gmail.com");
	list << contactInfo("Jim Smith", "jim.smith@smith.org");
	map["list"] = list;

	QString _template = "{{#list}}{{#counter}}#{{count}} {{name}} {{email}}{{/counter}}\n{{/list}}";

	Mustache::Renderer renderer;
	CounterContext context(map);
	QString output = renderer.render(_template, &context);
	QCOMPARE(output, QString("#1 Rob Knight robertknight@gmail.com\n"
	                         "#2 Jim Smith jim.smith@smith.org\n"));
}

// Create a QCoreApplication for the test.  In Qt 5 this can be
// done with QTEST_GUILESS_MAIN().
int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	TestMustache testObject;
	return QTest::qExec(&testObject, argc, argv);
}

