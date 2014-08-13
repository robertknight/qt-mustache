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

#include <QDir>
#include <QFile>
#include <QHash>
#include <QString>

#if QT_VERSION >= 0x050000
    #include <QJsonDocument>
    #include <QJsonObject>
    #include <QJsonArray>
#endif // QT_VERSION >= 0x050000

// To be able to use QHash<QString, QString> in QFETCH(..).
typedef QHash<QString, QString> PartialsHash;
Q_DECLARE_METATYPE(PartialsHash)

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
	// test changing the markers within a template
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

	// test changing the default markers
	renderer.setTagMarkers("%", "%");
	output = renderer.render("%name%'s phone number is %phone%", &context);
	QCOMPARE(output, QString("John Smith's phone number is 01234 567890"));

	renderer.setTagMarkers("{{", "}}");
	output = renderer.render("{{== ==}}", &context);
	QCOMPARE(renderer.error(), QString("Custom delimiters may not contain '='."));
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
	map["unescape"] = "One &amp; Two &quot;quoted&quot;";
	map["raw"] = "<b>foo</b>";

	QString _template = "{{escape}} {{&unescape}} {{{raw}}}";

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, QString("&lt;b&gt;foo&lt;/b&gt; One & Two \"quoted\" <b>foo</b>"));
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

void TestMustache::testHelpers()
{
	QVariantHash args;
	args.insert("name", "Jim Smith");
	args.insert("age", 42);

	QString output = Mustache::renderTemplate("Hello {{name}}, you are {{age}}", args);
	QCOMPARE(output, QString("Hello Jim Smith, you are 42"));
}

void TestMustache::testIncompleteTag()
{
	QVariantHash args;
	args.insert("name", "Jim Smith");

	QString output = Mustache::renderTemplate("Hello {{name}}, you are {", args);
	QCOMPARE(output, QString("Hello Jim Smith, you are {"));

	output = Mustache::renderTemplate("Hello {{name}}, you are {{", args);
	QCOMPARE(output, QString("Hello Jim Smith, you are {{"));

	output = Mustache::renderTemplate("Hello {{name}}, you are {{}", args);
	QCOMPARE(output, QString("Hello Jim Smith, you are {{}"));
}

void TestMustache::testIncompleteSection()
{
	QVariantHash args;
	args.insert("list", QVariantList() << QVariantHash());

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(args);
	QString output = renderer.render("{{#list}}", &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), QString("No matching end tag found for section"));

	output = renderer.render("{{^list}}", &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), QString("No matching end tag found for inverted section"));

	output = renderer.render("{{/list}}", &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), QString("Unexpected end tag"));

	output = renderer.render("{{#list}}{{/foo}}", &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), QString("Tag start/end key mismatch"));
}

static QString decorate(const QString& text, Mustache::Renderer* r, Mustache::Context* ctx)
{
	return "~" + r->render(text, ctx) + "~";
}

void TestMustache::testLambda()
{
	QVariantHash args;
	args["text"] = "test";
	args["fn"] = QVariant::fromValue(Mustache::QtVariantContext::fn_t(decorate));
	QString output = Mustache::renderTemplate("{{#fn}}{{text}}{{/fn}}", args);
	QCOMPARE(output, QString("~test~"));
}

#if QT_VERSION >= 0x050000 // JSON classes only in Qt 5+.

void TestMustache::testConformance_data()
{
	QTest::addColumn<QVariantMap>("data");
	QTest::addColumn<QString>("template_");
	QTest::addColumn<QHash<QString, QString> >("partials");
	QTest::addColumn<QString>("expected");

	QDir specsDir = QDir(".");

	foreach (const QString &fileName, specsDir.entryList(QStringList() << "*.json")) {
		QFile file(specsDir.filePath(fileName));
		QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(fileName + ": " + file.errorString()));

		QJsonDocument document = QJsonDocument::fromJson(file.readAll());
		QJsonArray testCaseValues = document.object()["tests"].toArray();

		foreach (const QJsonValue &testCaseValue, testCaseValues) {
			QJsonObject testCaseObject = testCaseValue.toObject();

			QString name = fileName + " - " + testCaseObject["name"].toString();
			QVariantMap data = testCaseObject["data"].toObject().toVariantMap();
			QString template_ = testCaseObject["template"].toString();
			QJsonObject partialsObject = testCaseObject["partials"].toObject();
			PartialsHash partials;
			foreach (const QString &partialName, partialsObject.keys()) {
				partials.insert(partialName, partialsObject[partialName].toString());
			}
			QString expected = testCaseObject["expected"].toString();

			QTest::newRow(qPrintable(name)) << data << template_ << partials << expected;
		}
	}
}

/*
 * This test will run once for each test case defined in version 1.1.2 of the
 * Mustache specification [1].
 *
 * [1] https://github.com/mustache/spec/tree/v1.1.2/specs
 */
void TestMustache::testConformance()
{
	QFETCH(QVariantMap, data);
	QFETCH(QString, template_);
	QFETCH(PartialsHash, partials);
	QFETCH(QString, expected);

	Mustache::Renderer renderer;
	Mustache::PartialMap partialsMap(partials);
	Mustache::QtVariantContext context(data, &partialsMap);

	QString output = renderer.render(template_, &context);

	// We currently fails these spec tests :(
	QEXPECT_FAIL("comments.json - Standalone", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Indented Standalone", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Standalone Line Endings", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Standalone Without Previous Line", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Standalone Without Newline", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Multiline Standalone", "TODO", Abort);
	QEXPECT_FAIL("comments.json - Indented Multiline Standalone", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Sections", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Inverted Sections", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Partial Inheritence", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Post-Partial Behavior", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Standalone Tag", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Indented Standalone Tag", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Standalone Line Endings", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Standalone Without Previous Line", "TODO", Abort);
	QEXPECT_FAIL("delimiters.json - Standalone Without Newline", "TODO", Abort);
	QEXPECT_FAIL("interpolation.json - Dotted Names - Basic Interpolation", "TODO", Abort);
	QEXPECT_FAIL("interpolation.json - Dotted Names - Triple Mustache Interpolation", "TODO", Abort);
	QEXPECT_FAIL("interpolation.json - Dotted Names - Ampersand Interpolation", "TODO", Abort);
	QEXPECT_FAIL("interpolation.json - Dotted Names - Arbitrary Depth", "TODO", Abort);
	QEXPECT_FAIL("interpolation.json - Dotted Names - Initial Resolution", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Context", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Doubled", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Dotted Names - Truthy", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Standalone Lines", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Standalone Indented Lines", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Standalone Line Endings", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Standalone Without Previous Line", "TODO", Abort);
	QEXPECT_FAIL("inverted.json - Standalone Without Newline", "TODO", Abort);
	QEXPECT_FAIL("partials.json - Standalone Line Endings", "TODO", Abort);
	QEXPECT_FAIL("partials.json - Standalone Without Previous Line", "TODO", Abort);
	QEXPECT_FAIL("partials.json - Standalone Without Newline", "TODO", Abort);
	QEXPECT_FAIL("partials.json - Standalone Indentation", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Context", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Deeply Nested Contexts", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Doubled", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Implicit Iterator - String", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Implicit Iterator - Integer", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Implicit Iterator - Decimal", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Dotted Names - Truthy", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Standalone Lines", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Indented Standalone Lines", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Standalone Line Endings", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Standalone Without Previous Line", "TODO", Abort);
	QEXPECT_FAIL("sections.json - Standalone Without Newline", "TODO", Abort);

	// Not applicable: Only ruby/perl/js/php/python/clojure available in spec.
	QEXPECT_FAIL("~lambdas.json - Interpolation", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Interpolation - Expansion", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Interpolation - Alternate Delimiters", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Interpolation - Multiple Calls", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Escaping", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Section", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Section - Expansion", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Section - Alternate Delimiters", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Section - Multiple Calls", "Expected failure", Abort);
	QEXPECT_FAIL("~lambdas.json - Inverted Section", "Expected failure", Abort);

	QCOMPARE(output, expected);
}

#endif // QT_VERSION >= 0x050000

// Create a QCoreApplication for the test.  In Qt 5 this can be
// done with QTEST_GUILESS_MAIN().
int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	TestMustache testObject;
	return QTest::qExec(&testObject, argc, argv);
}

