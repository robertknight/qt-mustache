/*
  Benchmarks for qt-mustache performance-critical paths.
*/

#include "mustache.h"

#include <QHash>
#include <QString>
#include <QTest>

using Qt::StringLiterals::operator""_s;

class BenchMustache : public QObject
{
	Q_OBJECT

private Q_SLOTS:
	// ── escapeHtml ──────────────────────────────────────────────

	void escapeHtml_noSpecialChars()
	{
		const QString input = u"The quick brown fox jumps over the lazy dog 1234567890"_s;
		QBENCHMARK {
			auto result = Mustache::escapeHtml(input);
			Q_UNUSED(result);
		}
	}

	void escapeHtml_fewSpecialChars()
	{
		const QString input = u"Hello <b>world</b> & \"friends\""_s;
		QBENCHMARK {
			auto result = Mustache::escapeHtml(input);
			Q_UNUSED(result);
		}
	}

	void escapeHtml_manySpecialChars()
	{
		const QString input = u"<a href=\"url?x=1&y=2&z=3\"><b>\"bold\" & <i>\"italic\"</i></b></a>"_s;
		QBENCHMARK {
			auto result = Mustache::escapeHtml(input);
			Q_UNUSED(result);
		}
	}

	void escapeHtml_largeString()
	{
		QString input;
		input.reserve(10000);
		for (int i = 0; i < 200; ++i)
			input += u"Hello <b>world</b> & \"friends\" "_s;
		QBENCHMARK {
			auto result = Mustache::escapeHtml(input);
			Q_UNUSED(result);
		}
	}

	// ── unescapeHtml ────────────────────────────────────────────

	void unescapeHtml_noEntities()
	{
		const QString input = u"The quick brown fox jumps over the lazy dog 1234567890"_s;
		QBENCHMARK {
			auto result = Mustache::unescapeHtml(input);
			Q_UNUSED(result);
		}
	}

	void unescapeHtml_someEntities()
	{
		const QString input = u"&lt;b&gt;hello&lt;/b&gt; &amp; &quot;world&quot;"_s;
		QBENCHMARK {
			auto result = Mustache::unescapeHtml(input);
			Q_UNUSED(result);
		}
	}

	void unescapeHtml_largeString()
	{
		QString input;
		input.reserve(10000);
		for (int i = 0; i < 200; ++i)
			input += u"&lt;b&gt;hello&lt;/b&gt; &amp; wo "_s;
		QBENCHMARK {
			auto result = Mustache::unescapeHtml(input);
			Q_UNUSED(result);
		}
	}

	// ── render ──────────────────────────────────────────────────

	void render_simpleSubstitution()
	{
		QVariantHash args;
		args[u"name"_s] = u"John Smith"_s;
		args[u"age"_s] = 42;
		args[u"city"_s] = u"Berlin"_s;
		const QString tmpl = u"Name: {{name}}, Age: {{age}}, City: {{city}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_htmlEscaping()
	{
		QVariantHash args;
		args[u"html"_s] = u"<script>alert(\"xss\")</script>"_s;
		const QString tmpl = u"Safe: {{html}}, Raw: {{{html}}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_listIteration()
	{
		QVariantList items;
		for (int i = 0; i < 100; ++i) {
			QVariantHash item;
			item[u"name"_s] = u"Item %1"_s.arg(i);
			item[u"value"_s] = i * 10;
			items.append(item);
		}
		QVariantHash args;
		args[u"items"_s] = items;
		const QString tmpl = u"{{#items}}{{name}}: {{value}}\n{{/items}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_nestedSections()
	{
		QVariantList groups;
		for (int g = 0; g < 10; ++g) {
			QVariantList members;
			for (int m = 0; m < 10; ++m) {
				QVariantHash member;
				member[u"name"_s] = u"Person %1-%2"_s.arg(g).arg(m);
				members.append(member);
			}
			QVariantHash group;
			group[u"title"_s] = u"Group %1"_s.arg(g);
			group[u"members"_s] = members;
			groups.append(group);
		}
		QVariantHash args;
		args[u"groups"_s] = groups;
		const QString tmpl =
		    u"{{#groups}}== {{title}} ==\n{{#members}}  - {{name}}\n{{/members}}{{/groups}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_partials()
	{
		QVariantList items;
		for (int i = 0; i < 50; ++i) {
			QVariantHash item;
			item[u"label"_s] = u"Entry %1"_s.arg(i);
			items.append(item);
		}
		QVariantHash args;
		args[u"items"_s] = items;

		QHash<QString, QString> partials;
		partials[u"row"_s] = u"[{{label}}] "_s;

		const QString tmpl = u"{{#items}}{{>row}}{{/items}}"_s;

		Mustache::Renderer renderer;
		Mustache::PartialMap partialMap(partials);
		Mustache::QtVariantContext context(args, &partialMap);

		QBENCHMARK {
			auto result = renderer.render(tmpl, &context);
			Q_UNUSED(result);
		}
	}

	void render_largeTemplate()
	{
		QVariantHash args;
		for (int i = 0; i < 50; ++i)
			args[u"key%1"_s.arg(i)] = u"value%1"_s.arg(i);

		QString tmpl;
		tmpl.reserve(2000);
		for (int i = 0; i < 50; ++i)
			tmpl += u"{{key%1}} "_s.arg(i);

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_invertedSections()
	{
		QVariantHash args;
		args[u"show"_s] = false;
		args[u"items"_s] = QVariantList{};
		const QString tmpl =
		    u"{{^show}}Hidden{{/show}} {{^items}}No items{{/items}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}

	void render_dotNotation()
	{
		QVariantHash inner;
		inner[u"city"_s] = u"Berlin"_s;
		QVariantHash middle;
		middle[u"location"_s] = inner;
		QVariantHash args;
		args[u"person"_s] = middle;
		const QString tmpl = u"City: {{person.location.city}}"_s;

		QBENCHMARK {
			auto result = Mustache::renderTemplate(tmpl, args);
			Q_UNUSED(result);
		}
	}
};

QTEST_GUILESS_MAIN(BenchMustache)
#include "bench_mustache.moc"
