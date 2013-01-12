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

#include "mustache.h"

#ifdef USE_QT
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QTextStream>

QString& adapt(QString& str)
{
	return str;
}
const QString& adapt(const QString& str)
{
	return str;
}
#else
#include "stlstringadapter.h"
#define Q_UNUSED(x) (void)x;
#endif

#include <string.h>

using namespace Mustache;

#ifdef USE_QT
StringType Mustache::renderTemplate(const StringType& templateString, const QVariantHash& args)
{
	Mustache::QtVariantContext context(args);
	Mustache::Renderer renderer;
	return renderer.render(templateString, &context);
}
#endif

StringType escapeHtml(const StringType& input)
{
	StringType escaped(input);
	for (int i=0; i < adapt(escaped).count();) {
		const char* replacement = 0;
		unsigned short ch = adapt(escaped).at(i).unicode();
		if (ch == '&') {
			replacement = "&amp;";
		} else if (ch == '<') {
			replacement = "&lt;";
		} else if (ch == '>') {
			replacement = "&gt;";
		} else if (ch == '"') {
			replacement = "&quot;";
		}
		if (replacement) {
			adapt(escaped).replace(i, 1, replacement);
			i += strlen(replacement);
		} else {
			++i;
		}
	}
	return escaped;
}

StringType unescapeHtml(const StringType& escaped)
{
	StringType unescaped(escaped);
	adapt(unescaped).replace("&lt;", "<");
	adapt(unescaped).replace("&gt;", ">");
	adapt(unescaped).replace("&amp;", "&");
	adapt(unescaped).replace("&quot;", "\"");
	return unescaped;
}

Context::Context(PartialResolver* resolver)
	: m_partialResolver(resolver)
{}

PartialResolver* Context::partialResolver() const
{
	return m_partialResolver;
}

StringType Context::partialValue(const StringType& key) const
{
	if (!m_partialResolver) {
		return StringType();
	}
	return m_partialResolver->getPartial(key);
}

bool Context::canEval(const StringType&) const
{
	return false;
}

StringType Context::eval(const StringType& key, const StringType& _template, Renderer* renderer)
{
	Q_UNUSED(key);
	Q_UNUSED(_template);
	Q_UNUSED(renderer);

	return StringType();
}

#ifdef USE_QT
QtVariantContext::QtVariantContext(const QVariant& root, PartialResolver* resolver)
	: Context(resolver)
{
	m_contextStack.push(root);
}

QVariant variantMapValue(const QVariant& value, const StringType& key)
{
	if (value.userType() == QVariant::Map) {
		return value.toMap().value(key);
	} else {
		return value.toHash().value(key);
	}
}

QVariant QtVariantContext::value(const StringType& key) const
{
	for (int i = m_contextStack.size()-1; i >= 0; i--) {
		QVariant value = variantMapValue(m_contextStack.at(i), key);
		if (!value.isNull()) {
			return value;
		}
	}
	return QVariant();
}

bool QtVariantContext::isFalse(const StringType& key) const
{
	QVariant value = this->value(key);
	switch (value.userType()) {
	case QVariant::Bool:
		return !value.toBool();
	case QVariant::List:
		return value.toList().isEmpty();
	default:
		return value.toString().isEmpty();
	}
}

StringType QtVariantContext::stringValue(const StringType& key) const
{
	if (isFalse(key)) {
		return StringType();
	}
	return value(key).toString();
}

void QtVariantContext::push(const StringType& key, int index)
{
	QVariant mapItem = value(key);
	if (index == -1) {
		m_contextStack << mapItem;
	} else {
		QVariantList list = mapItem.toList();
		m_contextStack << list.value(index, QVariant());
	}
}

void QtVariantContext::pop()
{
	m_contextStack.pop();
}

int QtVariantContext::listCount(const StringType& key) const
{
	if (value(key).userType() == QVariant::List) {
		return value(key).toList().count();
	}
	return 0;
}
#endif

#ifdef USE_QT
PartialMap::PartialMap(const QHash<StringType, StringType>& partials)
	: m_partials(partials)
{}

StringType PartialMap::getPartial(const StringType& name)
{
	return m_partials.value(name);
}
#endif

#ifdef USE_QT
PartialFileLoader::PartialFileLoader(const StringType& basePath)
	: m_basePath(basePath)
{}

StringType PartialFileLoader::getPartial(const StringType& name)
{
	if (!m_cache.contains(name)) {
		StringType path = m_basePath + '/' + name + ".mustache";
		QFile file(path);
		if (file.open(QIODevice::ReadOnly)) {
			QTextStream stream(&file);
			m_cache.insert(name, stream.readAll());
		}
	}
	return m_cache.value(name);
}
#endif

Renderer::Renderer()
	: m_errorPos(-1)
	, m_defaultTagStartMarker("{{")
	, m_defaultTagEndMarker("}}")
{
}

StringType Renderer::error() const
{
	return m_error;
}

int Renderer::errorPos() const
{
	return m_errorPos;
}

StringType Renderer::errorPartial() const
{
	return m_errorPartial;
}

StringType Renderer::render(const StringType& _template, Context* context)
{
	m_tagStartMarker = m_defaultTagStartMarker;
	m_tagEndMarker = m_defaultTagEndMarker;

	return render(_template, 0, _template.length(), context);
}

StringType Renderer::render(const StringType& _template, int startPos, int endPos, Context* context)
{
	setError(StringType(), -1);

	StringType output;
	int lastTagEnd = startPos;

	while (m_errorPos == -1) {
		Tag tag = findTag(_template, lastTagEnd, endPos);
		if (tag.type == Tag::Null) {
			output += adapt(_template).midRef(lastTagEnd, endPos - lastTagEnd);
			break;
		}
		output += adapt(_template).midRef(lastTagEnd, tag.start - lastTagEnd);
		switch (tag.type) {
		case Tag::Value:
		{
			StringType value = context->stringValue(tag.key);
			if (tag.escapeMode == Tag::Escape) {
				value = escapeHtml(value);
			} else if (tag.escapeMode == Tag::Unescape) {
				value = unescapeHtml(value);
			}
			output += value;
			lastTagEnd = tag.end;
		}
		break;
		case Tag::SectionStart:
		{
			Tag endTag = findEndTag(_template, tag, endPos);
			if (endTag.type == Tag::Null) {
				setError("No matching end tag found for section", tag.start);
			}
			int listCount = context->listCount(tag.key);
			if (listCount > 0) {
				for (int i=0; i < listCount; i++) {
					context->push(tag.key, i);
					output += render(_template, tag.end, endTag.start, context);
					context->pop();
				}
			} else if (context->canEval(tag.key)) {
				output += context->eval(tag.key, adapt(_template).mid(tag.end, endTag.start - tag.end), this);
			} else if (!context->isFalse(tag.key)) {
				context->push(tag.key);
				output += render(_template, tag.end, endTag.start, context);
				context->pop();
			}
			lastTagEnd = endTag.end;
		}
		break;
		case Tag::InvertedSectionStart:
		{
			Tag endTag = findEndTag(_template, tag, endPos);
			if (endTag.type == Tag::Null) {
				setError("No matching end tag found for inverted section", tag.start);
			}
			if (context->isFalse(tag.key)) {
				output += render(_template, tag.end, endTag.start, context);
			}
			lastTagEnd = endTag.end;
		}
		break;
		case Tag::SectionEnd:
			setError("Unexpected end tag", tag.start);
			lastTagEnd = tag.end;
			break;
		case Tag::Partial:
		{
			m_partialStack.push(tag.key);

			StringType partial = context->partialValue(tag.key);
			output += render(partial, 0, partial.length(), context);
			lastTagEnd = tag.end;

			m_partialStack.pop();
		}
		break;
		case Tag::SetDelimiter:
			lastTagEnd = tag.end;
			break;
		case Tag::Comment:
			lastTagEnd = tag.end;
			break;
		case Tag::Null:
			break;
		}
	}

	return output;
}

void Renderer::setError(const StringType& error, int pos)
{
	m_error = error;
	m_errorPos = pos;

	if (!m_partialStack.empty())
	{
		m_errorPartial = m_partialStack.top();
	}
}

Tag Renderer::findTag(const StringType& content, int pos, int endPos)
{
	int tagStartPos = adapt(content).indexOf(m_tagStartMarker, pos);
	if (tagStartPos == -1 || tagStartPos >= endPos) {
		return Tag();
	}

	int tagEndPos = adapt(content).indexOf(m_tagEndMarker, tagStartPos + m_tagStartMarker.length()) + m_tagEndMarker.length();
	if (tagEndPos == -1) {
		return Tag();
	}

	Tag tag;
	tag.type = Tag::Value;
	tag.start = tagStartPos;
	tag.end = tagEndPos;

	pos = tagStartPos + m_tagStartMarker.length();
	endPos = tagEndPos - m_tagEndMarker.length();

	unsigned short typeChar = adapt(content).at(pos).unicode();

	if (typeChar == '#') {
		tag.type = Tag::SectionStart;
		tag.key = readTagName(content, pos+1, endPos);
	} else if (typeChar == '^') {
		tag.type = Tag::InvertedSectionStart;
		tag.key = readTagName(content, pos+1, endPos);
	} else if (typeChar == '/') {
		tag.type = Tag::SectionEnd;
		tag.key = readTagName(content, pos+1, endPos);
	} else if (typeChar == '!') {
		tag.type = Tag::Comment;
	} else if (typeChar == '>') {
		tag.type = Tag::Partial;
		tag.key = readTagName(content, pos+1, endPos);
	} else if (typeChar == '=') {
		tag.type = Tag::SetDelimiter;
		readSetDelimiter(content, pos+1, tagEndPos - m_tagEndMarker.length());
	} else {
		if (typeChar == '&') {
			tag.escapeMode = Tag::Unescape;
			++pos;
		} else if (typeChar == '{') {
			tag.escapeMode = Tag::Raw;
			++pos;
			int endTache = adapt(content).indexOf('}', pos);
			if (endTache == tag.end - adapt(m_tagEndMarker).length()) {
				++tag.end;
			} else {
				endPos = endTache;
			}
		}
		tag.type = Tag::Value;
		tag.key = readTagName(content, pos, endPos);
	}

	return tag;
}

StringType Renderer::readTagName(const StringType& content, int pos, int endPos)
{
	StringType name;
	name.reserve(endPos - pos);
	while (adapt(content).at(pos).isSpace()) {
		++pos;
	}
	while (!adapt(content).at(pos).isSpace() && pos < endPos) {
		name += adapt(content).at(pos);
		++pos;
	}
	return name;
}

void Renderer::readSetDelimiter(const StringType& content, int pos, int endPos)
{
	StringType startMarker;
	StringType endMarker;

	while (!adapt(content).at(pos).isSpace() && pos < endPos) {
		if (adapt(content).at(pos) == '=') {
			setError("Custom delimiters may not contain '=' or spaces.", pos);
			return;
		}
		startMarker += adapt(content).at(pos);
		++pos;
	}

	while (adapt(content).at(pos).isSpace() && pos < endPos) {
		++pos;
	}

	while (pos < endPos - 1) {
		if (adapt(content).at(pos) == '=' || adapt(content).at(pos).isSpace()) {
			setError("Custom delimiters may not contain '=' or spaces.", pos);
			return;
		}
		endMarker += adapt(content).at(pos);
		++pos;
	}

	m_tagStartMarker = startMarker;
	m_tagEndMarker = endMarker;
}

Tag Renderer::findEndTag(const StringType& content, const Tag& startTag, int endPos)
{
	int tagDepth = 1;
	int pos = startTag.end;

	while (true) {
		Tag nextTag = findTag(content, pos, endPos);
		if (nextTag.type == Tag::Null) {
			return nextTag;
		} else if (nextTag.type == Tag::SectionStart || nextTag.type == Tag::InvertedSectionStart) {
			++tagDepth;
		} else if (nextTag.type == Tag::SectionEnd) {
			--tagDepth;
			if (tagDepth == 0) {
				if (nextTag.key != startTag.key) {
					setError("Tag start/end key mismatch", nextTag.start);
				}
				return nextTag;
			}
		}
		pos = nextTag.end;
	}

	return Tag();
}

void Renderer::setTagMarkers(const StringType& startMarker, const StringType& endMarker)
{
	m_defaultTagStartMarker = startMarker;
	m_defaultTagEndMarker = endMarker;
}

