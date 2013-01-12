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

#include <stack>

#define USE_QT

#ifdef USE_QT
#include <QtCore/QStack>
#include <QtCore/QString>
#include <QtCore/QVariant>
typedef QString StringType;
#else
#include <string>
typedef std::string StringType;
#endif

namespace Mustache
{

class PartialResolver;
class Renderer;

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
	virtual ~Context() {}

	/** Returns a string representation of the value for @p key in the current context.
	  * This is used to replace a Mustache value tag.
	  */
	virtual StringType stringValue(const StringType& key) const = 0;

	/** Returns true if the value for @p key is 'false' or an empty list.
	  * 'False' values typically include empty strings, the boolean value false etc.
	  *
	  * When processing a section Mustache tag, the section is not rendered if the key
	  * is false, or for an inverted section tag, the section is only rendered if the key
	  * is false.
	  */
	virtual bool isFalse(const StringType& key) const = 0;

	/** Returns the number of items in the list value for @p key or 0 if
	  * the value for @p key is not a list.
	  */
	virtual int listCount(const StringType& key) const = 0;

	/** Set the current context to the value for @p key.
	  * If index is >= 0, set the current context to the @p index'th value
	  * in the list value for @p key.
	  */
	virtual void push(const StringType& key, int index = -1) = 0;

	/** Exit the current context. */
	virtual void pop() = 0;

	/** Returns the partial template for a given @p key. */
	StringType partialValue(const StringType& key) const;

	/** Returns the partial resolver passed to the constructor. */
	PartialResolver* partialResolver() const;

	/** Returns true if eval() should be used to render section tags using @p key.
	 * If canEval() returns true for a key, the renderer will pass the literal, unrendered
	 * block of text for the section to eval() and replace the section with the result.
	 *
	 * canEval() and eval() are equivalents for callable objects (eg. lambdas) in other
	 * Mustache implementations.
	 *
	 * The default implementation always returns false.
	 */
	virtual bool canEval(const StringType& key) const;

	/** Callback used to render a template section with the given @p key.
	 * @p renderer will substitute the original section tag with the result of eval().
	 *
	 * The default implementation returns an empty string.
	 */
	virtual StringType eval(const StringType& key, const StringType& _template, Renderer* renderer);

private:
	PartialResolver* m_partialResolver;
};

#ifdef USE_QT
/** A context implementation which wraps a QVariantHash or QVariantMap. */
class QtVariantContext : public Context
{
public:
	/** Construct a QtVariantContext which wraps a dictionary in a QVariantHash
	 * or a QVariantMap.
	 */
	explicit QtVariantContext(const QVariant& root, PartialResolver* resolver = 0);

	virtual StringType stringValue(const StringType& key) const;
	virtual bool isFalse(const StringType& key) const;
	virtual int listCount(const StringType& key) const;
	virtual void push(const StringType& key, int index = -1);
	virtual void pop();

private:
	QVariant value(const StringType& key) const;

	QStack<QVariant> m_contextStack;
};
#endif

/** Interface for fetching template partials. */
class PartialResolver
{
public:
	virtual ~PartialResolver() {}

	/** Returns the partial template with a given @p name. */
	virtual StringType getPartial(const StringType& name) = 0;
};

#ifdef USE_QT
/** A simple partial fetcher which returns templates from a map of (partial name -> template)
  */
class PartialMap : public PartialResolver
{
public:
	explicit PartialMap(const QHash<StringType,StringType>& partials);

	virtual StringType getPartial(const StringType& name);

private:
	QHash<StringType, StringType> m_partials;
};

/** A partial fetcher when loads templates from '<name>.mustache' files
 * in a given directory.
 *
 * Once a partial has been loaded, it is cached for future use.
 */
class PartialFileLoader : public PartialResolver
{
public:
	explicit PartialFileLoader(const StringType& basePath);

	virtual StringType getPartial(const StringType& name);

private:
	StringType m_basePath;
	QHash<StringType, StringType> m_cache;
};
#endif

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

	enum EscapeMode
	{
		Escape,
		Unescape,
		Raw
	};

	Tag()
		: type(Null)
		, start(0)
		, end(0)
		, escapeMode(Escape)
	{}

	Type type;
	StringType key;
	int start;
	int end;
	EscapeMode escapeMode;
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
	StringType render(const StringType& _template, Context* context);

	/** Returns a message describing the last error encountered by the previous
	  * render() call.
	  */
	StringType error() const;

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
	StringType errorPartial() const;

	/** Sets the default tag start and end markers.
	  * This can be overridden within a template.
	  */
	void setTagMarkers(const StringType& startMarker, const StringType& endMarker);

private:
	StringType render(const StringType& _template, int startPos, int endPos, Context* context);

	Tag findTag(const StringType& content, int pos, int endPos);
	Tag findEndTag(const StringType& content, const Tag& startTag, int endPos);
	void setError(const StringType& error, int pos);

	void readSetDelimiter(const StringType& content, int pos, int endPos);
	static StringType readTagName(const StringType& content, int pos, int endPos);

	std::stack<StringType> m_partialStack;
	StringType m_error;
	int m_errorPos;
	StringType m_errorPartial;

	StringType m_tagStartMarker;
	StringType m_tagEndMarker;

	StringType m_defaultTagStartMarker;
	StringType m_defaultTagEndMarker;
};

#ifdef USE_QT
/** A convenience function which renders a template using the given data. */
StringType renderTemplate(const StringType& templateString, const QVariantHash& args);
#endif

};
