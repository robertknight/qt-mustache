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

#pragma once

#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Mustache
{

class PartialResolver;

/** Context is an interface that Mustache::Renderer::render() uses to
  * fetch substitutions for template tags.
  */
class Context
{
public:
	/** Create a context.  @p resolver is used to fetch the expansions for any {{>partial}} tags
	  * which appear in a template.
	  */
	explicit Context(PartialResolver* resolver = 0);

	/** Returns a string representation of the value for @p key in the current context.
	  * This is used to replace a Mustache value tag.
	  *
	  * If @p escape is true, the value should be HTML escaped, eg. using Qt::escape().
	  */
	virtual QString stringValue(const QString& key, bool escape) const = 0;

	/** Returns true if the value for @p key is 'false' or an empty list.
	  * 'False' values typically include empty strings, the boolean value false etc.
	  *
	  * When processing a section Mustache tag, the section is not rendered if the key
	  * is false, or for an inverted section tag, the section is only rendered if the key
	  * is false.
	  */
	virtual bool isFalse(const QString& key) const = 0;

	/** Returns the number of items in the list value for @p key or 0 if
	  * the value for @p key is not a list.
	  */
	virtual int listCount(const QString& key) const = 0;

	/** Set the current context to the value for @p key.
	  * If index is >= 0, set the current context to the @p index'th value
	  * in the list value for @p key.
	  */
	virtual void push(const QString& key, int index = -1) = 0;

	/** Exit the current context. */
	virtual void pop() = 0;

	/** Returns the partial template for a given @p key. */
	QString partialValue(const QString& key) const;

	/** Returns the partial resolver passed to the constructor. */
	PartialResolver* partialResolver() const;

private:
	PartialResolver* m_partialResolver;
};

/** A context implementation which wraps a QVariantMap. */
class QtVariantContext : public Context
{
public:
	typedef QHash<QString, QString> PartialMap;

	explicit QtVariantContext(const QVariantMap& root, PartialResolver* resolver = 0);

	virtual QString stringValue(const QString& key, bool escape) const;
	virtual bool isFalse(const QString& key) const;
	virtual int listCount(const QString& key) const;
	virtual void push(const QString& key, int index = -1);
	virtual void pop();

private:
	QVariant value(const QString& key) const;

	QStack<QVariantMap> m_contextStack;
};

/** Interface for fetching template partials. */
class PartialResolver
{
public:
	/** Returns the partial template with a given @p name. */
	virtual QString getPartial(const QString& name) = 0;
};

/** A simple partial fetcher which returns templates from a map of (partial name -> template)
  */
class PartialMap : public PartialResolver
{
public:
	explicit PartialMap(const QHash<QString,QString>& partials);

	virtual QString getPartial(const QString& name);

private:
	QHash<QString, QString> m_partials;
};

/** A partial fetcher when loads templates from '<name>.mustache' files
 * in a given directory. 
 * 
 * Once a partial has been loaded, it is cached for future use.
 */
class PartialFileLoader : public PartialResolver
{
public:
	explicit PartialFileLoader(const QString& basePath);

	virtual QString getPartial(const QString& name);

private:
	QString m_basePath;
	QHash<QString, QString> m_cache;
};

/** Holds properties of a tag in a mustache template. */
struct Tag
{
    enum Type
    {
        Null,
        Value, /// A {{key}} or {{{key}}} tag
        SectionStart, /// A {{#section}} tag
        InvertedSectionStart, /// An {{^inverted-section}} tag
        SectionEnd, /// A {{/section}} tag
        Partial, /// A {{^partial}} tag
        Comment, /// A {{! comment }} tag
        SetDelimiter /// A {{=<% %>=}} tag
    };

    Tag()
        : type(Null)
        , start(0)
        , end(0)
        , escape(true)
    {}

    Type type;
    QString key;
    int start;
    int end;
    bool escape;
};

/** Renders Mustache templates, replacing mustache tags with
  * values from a provided context.
  */
class Renderer
{
public:
	Renderer();

	/** Render a Mustache template, using @p context to fetch
	  * the values used to replace Mustache tags.
	  */
	QString render(const QString& _template, Context* context);

	/** Returns a message describing the last error encountered by the previous
	  * render() call.
	  */
	QString error() const;

	/** Returns the position in the template where the last error occurred
	  * when rendering the template or -1 if no error occurred.
	  *
	  * If the error occurred in a partial template, the returned position is the offset
	  * in the partial template.
	  */
	int errorPos() const;

	/** Returns the name of the partial where the error occurred, or an empty string
	 * if the error occurred in the main template.
	 */
	QString errorPartial() const;

private:
	QString render(const QString& _template, int startPos, int endPos, Context *context);

	Tag findTag(const QString& content, int pos, int endPos);
	Tag findEndTag(const QString& content, const Tag& startTag, int endPos);
	void setError(const QString& error, int pos);

	void readSetDelimiter(const QString& content, int pos, int endPos);
	static QString readTagName(const QString& content, int pos, int endPos);

	QStack<QString> m_partialStack;
	QString m_error;
	int m_errorPos;
	QString m_errorPartial;

	QString m_tagStartMarker;
	QString m_tagEndMarker;
};

};
