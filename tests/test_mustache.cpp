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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QString>

using Qt::StringLiterals::operator""_s;

// To be able to use QHash<QString, QString> in QFETCH(..).
using PartialsHash = QHash<QString, QString>;
Q_DECLARE_METATYPE(PartialsHash)

void TestMustache::testValues()
{
	QVariantHash map;
	map[u"name"_s] = u"John Smith"_s;
	map[u"age"_s] = 42;
	map[u"sex"_s] = u"Male"_s;
	map[u"company"_s] = u"Smith & Co"_s;
	map[u"signature"_s] = u"John Smith of <b>Smith & Co</b>"_s;
	map[u"alive"_s] = false;

	QString _template = u"Name: {{name}}, Age: {{age}}, Sex: {{sex}}, Alive: {{alive}}\n"_s
	                    u"Company: {{company}}\n"_s
	                    u"  {{{signature}}}"_s
	                    u"{{missing-key}}"_s;
	QString expectedOutput = u"Name: John Smith, Age: 42, Sex: Male, Alive: false\n"_s
	                         u"Company: Smith &amp; Co\n"_s
	                         u"  John Smith of <b>Smith & Co</b>"_s;

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);
}

void TestMustache::testFloatValues()
{
  QList<float> values = {-3., -0.5, 0., 0.5, 1., 1.5, 3.};

  for (auto value : values) {
    QVariantHash map;
    map[u"val"_s] = value;
    QString _template = u"Value: {{val}}"_s;
    QString expectedOutput = u"Value: "_s + QString::number(value);

    Mustache::Renderer renderer;
    Mustache::QtVariantContext context(map);
    QString output = renderer.render(_template, &context);

    QCOMPARE(output, expectedOutput);
  }
}

QVariantHash contactInfo(const QString& name, const QString& email)
{
	QVariantHash map;
	map[u"name"_s] = name;
	map[u"email"_s] = email;
	return map;
}

void TestMustache::testSections()
{
	QVariantHash map = contactInfo(u"John Smith"_s, u"john.smith@gmail.com"_s);
	QVariantList contacts;
	contacts << contactInfo(u"James Dee"_s, u"james@dee.org"_s);
	contacts << contactInfo(u"Jim Jones"_s, u"jim-jones@yahoo.com"_s);
	map[u"contacts"_s] = contacts;

	QString _template = u"Name: {{name}}, Email: {{email}}\n"_s
	                    u"{{#contacts}}  {{name}} - {{email}}\n{{/contacts}}"_s
	                    u"{{^contacts}}  No contacts{{/contacts}}"_s;

	QString expectedOutput = u"Name: John Smith, Email: john.smith@gmail.com\n"_s
	                         u"  James Dee - james@dee.org\n"_s
	                         u"  Jim Jones - jim-jones@yahoo.com\n"_s;

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);

	// test inverted sections
	map.remove(u"contacts"_s);
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);

	expectedOutput = u"Name: John Smith, Email: john.smith@gmail.com\n"_s
	                 u"  No contacts"_s;
	QCOMPARE(output, expectedOutput);

	// test with an empty list instead of an empty key
	map[u"contacts"_s] = QVariantHash();
	context = Mustache::QtVariantContext(map);
	output = renderer.render(_template, &context);
	QCOMPARE(output, expectedOutput);
}

void TestMustache::testSectionQString()
{
	QVariantHash data;
	data[u"text"_s] = u"test"_s;
	QString output = Mustache::renderTemplate(u"{{#text}}{{text}}{{/text}}"_s, data);
	QCOMPARE(output, u"test"_s);
}

void TestMustache::testFalsiness()
{
	Mustache::Renderer renderer;
	QVariantHash data;
	QString _template = u"{{#bool}}This should not be shown{{/bool}}"_s;

	// test falsiness of 0
	data[u"bool"_s] = 0;
	Mustache::QtVariantContext context = Mustache::QtVariantContext(data);
	QString output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0 evaluated as truthy");

	// test falsiness of 0u
	data[u"bool"_s] = 0u;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0u evaluated as truthy");

	// test falsiness of 0ll
	data[u"bool"_s] = 0ll;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0ll evaluated as truthy");

	// test falsiness of 0ull
	data[u"bool"_s] = 0ull;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0ull evaluated as truthy");

	// test falsiness of 0.0
	data[u"bool"_s] = 0.0;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0.0 evaluated as truthy");

	// test falsiness of 0.4
	data[u"bool"_s] = 0.4f;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(!output.isEmpty(), "0.4 evaluated as falsey");

	// test falsiness of 0.5
	data[u"bool"_s] = 0.5f;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(!output.isEmpty(), "0.5f evaluated as falsey");

	// test falsiness of 0.0f
	data[u"bool"_s] = 0.0f;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "0.0f evaluated as truthy");

	// test falsiness of '\0'
	data[u"bool"_s] = '\0';
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "'\0' evaluated as truthy");

	// test falsiness of 'false'
	data[u"bool"_s] = false;
	context = Mustache::QtVariantContext(data);
	output = renderer.render(_template, &context);
	QVERIFY2(output.isEmpty(), "'\0' evaluated as truthy");
}

void TestMustache::testContextLookup()
{
	QVariantHash fileMap;
	fileMap[u"dir"_s] = u"/home/robert"_s;
	fileMap[u"name"_s] = u"robert"_s;

	QVariantList files;
	QVariantHash file;
	file[u"name"_s] = u"test.pdf"_s;
	files << file;

	fileMap[u"files"_s] = files;

	QString _template = u"{{#files}}{{dir}}/{{name}}{{/files}}"_s;

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(fileMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, u"/home/robert/test.pdf"_s);
}

void TestMustache::testPartials()
{
	QHash<QString, QString> partials;
	partials[u"file-info"_s] = u"{{name}} {{size}} {{type}}\n"_s;

	QString _template = u"{{#files}}{{>file-info}}{{/files}}"_s;

	QVariantHash map;
	QVariantList fileList;

	QVariantHash file1;
	file1[u"name"_s] = u"mustache.pdf"_s;
	file1[u"size"_s] = u"200KB"_s;
	file1[u"type"_s] = u"PDF Document"_s;

	QVariantHash file2;
	file2[u"name"_s] = u"cv.doc"_s;
	file2[u"size"_s] = u"300KB"_s;
	file2[u"type"_s] = u"Microsoft Word Document"_s;

	fileList << file1 << file2;
	map[u"files"_s] = fileList;

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output,
	         u"mustache.pdf 200KB PDF Document\n"_s
	         u"cv.doc 300KB Microsoft Word Document\n"_s);
}

void TestMustache::testSetDelimiters()
{
	// test changing the markers within a template
	QVariantHash map;
	map[u"name"_s] = u"John Smith"_s;
	map[u"phone"_s] = u"01234 567890"_s;

	QString _template =
	    u"{{=<% %>=}}"_s
	    u"<%name%>{{ }}<%phone%>"_s
	    u"<%={{ }}=%>"_s
	    u" {{name}}<% %>{{phone}}"_s;

	QString expectedOutput = u"John Smith{{ }}01234 567890 John Smith<% %>01234 567890"_s;

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, expectedOutput);

	// test changing the default markers
	renderer.setTagMarkers(u"%"_s, u"%"_s);
	output = renderer.render(u"%name%'s phone number is %phone%"_s, &context);
	QCOMPARE(output, u"John Smith's phone number is 01234 567890"_s);

	renderer.setTagMarkers(u"{{"_s, u"}}"_s);
	output = renderer.render(u"{{== ==}}"_s, &context);
	QCOMPARE(renderer.error(), u"Custom delimiters may not contain '='."_s);
}

void TestMustache::testErrors()
{
	QVariantHash map;
	map[u"name"_s] = u"Jim Jones"_s;

	QHash<QString, QString> partials;
	partials[u"buggy-partial"_s] = u"--{{/one}}--"_s;

	QString _template = u"{{name}}"_s;

	Mustache::Renderer renderer;
	Mustache::PartialMap partialMap(partials);
	Mustache::QtVariantContext context(map, &partialMap);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, u"Jim Jones"_s);
	QCOMPARE(renderer.error(), QString());
	QCOMPARE(renderer.errorPos(), -1);

	_template = u"{{#one}} {{/two}}"_s;
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), u"Tag start/end key mismatch"_s);
	QCOMPARE(renderer.errorPos(), 9);
	QCOMPARE(renderer.errorPartial(), QString());

	_template = u"Hello {{>buggy-partial}}"_s;
	output = renderer.render(_template, &context);
	QCOMPARE(renderer.error(), u"Unexpected end tag"_s);
	QCOMPARE(renderer.errorPos(), 2);
	QCOMPARE(renderer.errorPartial(), u"buggy-partial"_s);
}

void TestMustache::testPartialFile()
{
	QString path = QCoreApplication::applicationDirPath();

	QVariantHash map = contactInfo(u"Jim Smith"_s, u"jim.smith@gmail.com"_s);

	QString _template = u"{{>partial}}"_s;

	Mustache::Renderer renderer;
	Mustache::PartialFileLoader partialLoader(path);
	Mustache::QtVariantContext context(map, &partialLoader);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, u"Jim Smith -- jim.smith@gmail.com\n"_s);
}

void TestMustache::testEscaping()
{
	QVariantHash map;
	map[u"escape"_s] = u"<b>foo</b>"_s;
	map[u"unescape"_s] = u"One &amp; Two &quot;quoted&quot;"_s;
	map[u"raw"_s] = u"<b>foo</b>"_s;

	QString _template = u"{{escape}} {{&unescape}} {{{raw}}}"_s;

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(map);
	QString output = renderer.render(_template, &context);

	QCOMPARE(output, u"&lt;b&gt;foo&lt;/b&gt; One & Two \"quoted\" <b>foo</b>"_s);
}

class CounterContext : public Mustache::QtVariantContext
{
public:
	int counter;

	CounterContext(const QVariantHash& map)
		: Mustache::QtVariantContext(map)
		, counter(0)
	{}

	bool canEval(const QString& key) const override {
		return key == u"counter"_s;
	}

	QString eval(const QString& key, const QString& _template, Mustache::Renderer* renderer) override {
		if (key == u"counter"_s) {
			++counter;
		}
		return renderer->render(_template, this);
	}

	QString stringValue(const QString& key) const override {
		if (key == u"count"_s) {
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
	list << contactInfo(u"Rob Knight"_s, u"robertknight@gmail.com"_s);
	list << contactInfo(u"Jim Smith"_s, u"jim.smith@smith.org"_s);
	map[u"list"_s] = list;

	QString _template = u"{{#list}}{{#counter}}#{{count}} {{name}} {{email}}{{/counter}}\n{{/list}}"_s;

	Mustache::Renderer renderer;
	CounterContext context(map);
	QString output = renderer.render(_template, &context);
	QCOMPARE(output, u"#1 Rob Knight robertknight@gmail.com\n"_s
	                 u"#2 Jim Smith jim.smith@smith.org\n"_s);
}

void TestMustache::testHelpers()
{
	QVariantHash args;
	args.insert(u"name"_s, u"Jim Smith"_s);
	args.insert(u"age"_s, 42);

	QString output = Mustache::renderTemplate(u"Hello {{name}}, you are {{age}}"_s, args);
	QCOMPARE(output, u"Hello Jim Smith, you are 42"_s);
}

void TestMustache::testIncompleteTag()
{
	QVariantHash args;
	args.insert(u"name"_s, u"Jim Smith"_s);

	QString output = Mustache::renderTemplate(u"Hello {{name}}, you are {"_s, args);
	QCOMPARE(output, u"Hello Jim Smith, you are {"_s);

	output = Mustache::renderTemplate(u"Hello {{name}}, you are {{"_s, args);
	QCOMPARE(output, u"Hello Jim Smith, you are {{"_s);

	output = Mustache::renderTemplate(u"Hello {{name}}, you are {{}"_s, args);
	QCOMPARE(output, u"Hello Jim Smith, you are {{}"_s);
}

void TestMustache::testIncompleteSection()
{
	QVariantHash args;
	args.insert(u"list"_s, QVariantList() << QVariantHash());

	Mustache::Renderer renderer;
	Mustache::QtVariantContext context(args);
	QString output = renderer.render(u"{{#list}}"_s, &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), u"No matching end tag found for section"_s);

	output = renderer.render(u"{{^list}}"_s, &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), u"No matching end tag found for inverted section"_s);

	output = renderer.render(u"{{/list}}"_s, &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), u"Unexpected end tag"_s);

	output = renderer.render(u"{{#list}}{{/foo}}"_s, &context);
	QCOMPARE(output, QString());
	QCOMPARE(renderer.error(), u"Tag start/end key mismatch"_s);
}

static QString decorate(const QString& text, Mustache::Renderer* r, Mustache::Context* ctx)
{
	return u"~"_s + r->render(text, ctx) + u"~"_s;
}

void TestMustache::testLambda()
{
	QVariantHash args;
	args[u"text"_s] = u"test"_s;
	args[u"fn"_s] = QVariant::fromValue(Mustache::QtVariantContext::fn_t(decorate));
	QString output = Mustache::renderTemplate(u"{{#fn}}{{text}}{{/fn}}"_s, args);
	QCOMPARE(output, u"~test~"_s);
}

void TestMustache::testQStringListIteration()
{
	QStringList list;
	list << u"str1"_s << u"str2"_s << u"str3"_s;
	QVariantHash args;
	args[u"list"_s] = list;
	QString output = Mustache::renderTemplate(u"{{#list}}{{.}}{{/list}}"_s, args);
	QCOMPARE(output, u"str1str2str3"_s);
}

void TestMustache::testDefaultEscapeModeRaw()
{
	QVariantHash map;
	map[u"html"_s] = u"<b>bold</b>"_s;
	map[u"entity"_s] = u"One &amp; Two"_s;

	// With Raw default, {{key}} should pass through unescaped
	Mustache::Renderer rawRenderer(Mustache::Tag::Raw);
	Mustache::QtVariantContext context(map);

	QString output = rawRenderer.render(u"{{html}}"_s, &context);
	QCOMPARE(output, u"<b>bold</b>"_s);

	// {{{key}}} is still raw
	output = rawRenderer.render(u"{{{html}}}"_s, &context);
	QCOMPARE(output, u"<b>bold</b>"_s);

	// {{&key}} still unescapes HTML entities
	output = rawRenderer.render(u"{{&entity}}"_s, &context);
	QCOMPARE(output, u"One & Two"_s);

	// Verify default Escape renderer still escapes
	Mustache::Renderer defaultRenderer;
	output = defaultRenderer.render(u"{{html}}"_s, &context);
	QCOMPARE(output, u"&lt;b&gt;bold&lt;/b&gt;"_s);

	// renderTemplate convenience function with Raw
	output = Mustache::renderTemplate(u"{{html}}"_s, map, Mustache::Tag::Raw);
	QCOMPARE(output, u"<b>bold</b>"_s);

	// renderTemplate convenience function without explicit mode still escapes
	output = Mustache::renderTemplate(u"{{html}}"_s, map);
	QCOMPARE(output, u"&lt;b&gt;bold&lt;/b&gt;"_s);
}

void TestMustache::testUnescapeHtml()
{
	QVariantHash args;
	args[u"s"_s] = u"&lt;&gt;&amp;&quot;&amp;quot;"_s;
	QString output = Mustache::renderTemplate(u"{{&s}}"_s, args);
	QCOMPARE(output, u"<>&\"&quot;"_s);
}

void TestMustache::testConformance_data()
{
	QTest::addColumn<QVariantMap>("data");
	QTest::addColumn<QString>("template_");
	QTest::addColumn<QHash<QString, QString> >("partials");
	QTest::addColumn<QString>("expected");

	QDir specsDir = QDir(u"."_s);

	const auto jsonFiles = specsDir.entryList(QStringList{u"*.json"_s});
	for (const QString &fileName : jsonFiles) {
		QFile file(specsDir.filePath(fileName));
		QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(fileName + u": "_s + file.errorString()));

		QJsonDocument document = QJsonDocument::fromJson(file.readAll());
		QJsonArray testCaseValues = document.object()[u"tests"_s].toArray();

		for (const QJsonValue &testCaseValue: testCaseValues) {
			QJsonObject testCaseObject = testCaseValue.toObject();

			QString name = fileName + u" - "_s + testCaseObject[u"name"_s].toString();
			QVariantMap data = testCaseObject[u"data"_s].toObject().toVariantMap();
			QString template_ = testCaseObject[u"template"_s].toString();
			QJsonObject partialsObject = testCaseObject[u"partials"_s].toObject();
			PartialsHash partials;
			for (const QString &partialName : partialsObject.keys()) {
				partials.insert(partialName, partialsObject[partialName].toString());
			}
			QString expected = testCaseObject[u"expected"_s].toString();

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

	QCOMPARE(output, expected);
}

// Create a QCoreApplication for the test.  In Qt 5 this can be
// done with QTEST_GUILESS_MAIN().
int main(int argc, char** argv)
{
	QCoreApplication app(argc, argv);
	TestMustache testObject;
	return QTest::qExec(&testObject, argc, argv);
}
