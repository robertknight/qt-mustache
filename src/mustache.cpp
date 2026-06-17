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

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QStringList>

using namespace Qt::StringLiterals;

using namespace Mustache;

QString Mustache::renderTemplate(const QString &templateString, const QVariantHash &args,
                                 Tag::EscapeMode defaultEscapeMode)
{
    Mustache::QtVariantContext context(args);
    Mustache::Renderer renderer(defaultEscapeMode);
    return renderer.render(templateString, &context);
}

QString Mustache::escapeHtml(const QString &input)
{
    QString escaped;
    escaped.reserve(input.size() + input.size() / 8);
    int pos = 0;
    const int len = input.size();
    while (pos < len) {
        const int start = pos;
        while (pos < len) {
            const char16_t ch = input.at(pos).unicode();
            if (ch == u'&' || ch == u'<' || ch == u'>' || ch == u'"') {
                break;
            }
            ++pos;
        }
        if (pos > start) {
            escaped += QStringView(input).mid(start, pos - start);
        }
        if (pos < len) {
            switch (input.at(pos).unicode()) {
            case u'&':
                escaped += u"&amp;"_s;
                break;
            case u'<':
                escaped += u"&lt;"_s;
                break;
            case u'>':
                escaped += u"&gt;"_s;
                break;
            case u'"':
                escaped += u"&quot;"_s;
                break;
            }
            ++pos;
        }
    }
    return escaped;
}

QString Mustache::unescapeHtml(const QString &escaped)
{
    QString unescaped(escaped);
    unescaped.replace(u"&lt;"_s, u"<"_s);
    unescaped.replace(u"&gt;"_s, u">"_s);
    unescaped.replace(u"&quot;"_s, u"\""_s);
    unescaped.replace(u"&amp;"_s, u"&"_s);
    return unescaped;
}

Context::Context(PartialResolver *resolver)
    : m_partialResolver(resolver)
{
}

PartialResolver *Context::partialResolver() const
{
    return m_partialResolver;
}

QString Context::partialValue(const QString &key) const
{
    if (!m_partialResolver) {
        return {};
    }
    return m_partialResolver->getPartial(key);
}

bool Context::canEval(const QString &) const
{
    return false;
}

QString Context::eval(const QString &key, const QString &_template, Renderer *renderer)
{
    Q_UNUSED(key);
    Q_UNUSED(_template);
    Q_UNUSED(renderer);

    return {};
}

QtVariantContext::QtVariantContext(const QVariant &root, PartialResolver *resolver)
    : Context(resolver)
{
    m_contextStack << root;
}

QVariant variantMapValue(const QVariant &value, const QString &key)
{
    if (value.userType() == QMetaType::QVariantMap) {
        return value.toMap().value(key);
    } else {
        return value.toHash().value(key);
    }
}

QVariant variantMapValueForKeyPath(const QVariant &value, const QStringList &keyPath)
{
    if (keyPath.count() > 1) {
        QVariant firstValue = variantMapValue(value, keyPath.first());
        return firstValue.isNull() ? QVariant() : variantMapValueForKeyPath(firstValue, keyPath.mid(1));
    } else if (!keyPath.isEmpty()) {
        return variantMapValue(value, keyPath.first());
    }
    return {};
}

QVariant QtVariantContext::value(const QString &key) const
{
    if (key == u"."_s && !m_contextStack.isEmpty()) {
        return m_contextStack.last();
    }
    QStringList keyPath = key.split(u"."_s);
    for (int i = m_contextStack.count() - 1; i >= 0; i--) {
        QVariant value = variantMapValueForKeyPath(m_contextStack.at(i), keyPath);
        if (!value.isNull()) {
            return value;
        }
    }
    return {};
}

bool QtVariantContext::isFalse(const QString &key) const
{
    QVariant value = this->value(key);
    switch (value.userType()) {
    case QMetaType::Double:
    case QMetaType::Float:
        // QVariant::toBool() rounds floats to the nearest int and then compares
        // against 0, which is not the falsiness behavior we want.
        return value.toDouble() == 0.;
    case QMetaType::QChar:
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::LongLong:
    case QMetaType::ULongLong:
    case QMetaType::Bool:
        return !value.toBool();
    case QMetaType::QVariantList:
    case QMetaType::QStringList:
        return value.toList().isEmpty();
    case QMetaType::QVariantHash:
        return value.toHash().isEmpty();
    case QMetaType::QVariantMap:
        return value.toMap().isEmpty();
    default:
        return value.toString().isEmpty();
    }
}

QString QtVariantContext::stringValue(const QString &key) const
{
    return value(key).toString();
}

void QtVariantContext::push(const QString &key, int index)
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

int QtVariantContext::listCount(const QString &key) const
{
    const QVariant &item = value(key);
    if (item.canConvert<QVariantList>() && item.userType() != QMetaType::QString) {
        return item.toList().count();
    }
    return 0;
}

bool QtVariantContext::canEval(const QString &key) const
{
    return value(key).canConvert<fn_t>();
}

QString QtVariantContext::eval(const QString &key, const QString &_template, Renderer *renderer)
{
    QVariant fn = value(key);
    if (fn.isNull()) {
        return {};
    }
    return fn.value<fn_t>()(_template, renderer, this);
}

PartialMap::PartialMap(const QHash<QString, QString> &partials)
    : m_partials(partials)
{
}

QString PartialMap::getPartial(const QString &name)
{
    return m_partials.value(name);
}

PartialFileLoader::PartialFileLoader(const QString &basePath)
    : m_basePath(basePath)
{
}

QString PartialFileLoader::getPartial(const QString &name)
{
    if (!m_cache.contains(name)) {
        const QString path = m_basePath + u'/' + name + u".mustache"_s;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_cache.insert(name, QString::fromUtf8(file.readAll()));
        }
    }
    return m_cache.value(name);
}

Renderer::Renderer(Tag::EscapeMode defaultEscapeMode)
    : m_errorPos(-1)
    , m_defaultTagStartMarker(u"{{"_s)
    , m_defaultTagEndMarker(u"}}"_s)
    , m_defaultEscapeMode(defaultEscapeMode)
{
}

QString Renderer::error() const
{
    return m_error;
}

int Renderer::errorPos() const
{
    return m_errorPos;
}

QString Renderer::errorPartial() const
{
    return m_errorPartial;
}

QString Renderer::render(const QString &_template, Context *context)
{
    m_error.clear();
    m_errorPos = -1;
    m_errorPartial.clear();

    m_tagStartMarker = m_defaultTagStartMarker;
    m_tagEndMarker = m_defaultTagEndMarker;

    return render(_template, 0, _template.length(), context);
}

QString Renderer::render(const QString &_template, int startPos, int endPos, Context *context)
{
    QString output;
    output.reserve(endPos - startPos);
    const QStringView templateView(_template);
    int lastTagEnd = startPos;

    while (m_errorPos == -1) {
        Tag tag = findTag(_template, lastTagEnd, endPos);
        if (tag.type == Tag::Null) {
            output += templateView.mid(lastTagEnd, endPos - lastTagEnd);
            break;
        }
        output += templateView.mid(lastTagEnd, tag.start - lastTagEnd);
        switch (tag.type) {
        case Tag::Value: {
            QString value = context->stringValue(tag.key);
            if (tag.escapeMode == Tag::Escape) {
                value = escapeHtml(value);
            } else if (tag.escapeMode == Tag::Unescape) {
                value = unescapeHtml(value);
            }
            output += value;
            lastTagEnd = tag.end;
        } break;
        case Tag::SectionStart: {
            Tag endTag = findEndTag(_template, tag, endPos);
            if (endTag.type == Tag::Null) {
                if (m_errorPos == -1) {
                    setError(u"No matching end tag found for section"_s, tag.start);
                }
            } else {
                int listCount = context->listCount(tag.key);
                if (listCount > 0) {
                    for (int i = 0; i < listCount; i++) {
                        context->push(tag.key, i);
                        output += render(_template, tag.end, endTag.start, context);
                        context->pop();
                    }
                } else if (context->canEval(tag.key)) {
                    output += context->eval(tag.key, _template.mid(tag.end, endTag.start - tag.end), this);
                } else if (!context->isFalse(tag.key)) {
                    context->push(tag.key);
                    output += render(_template, tag.end, endTag.start, context);
                    context->pop();
                }
                lastTagEnd = endTag.end;
            }
        } break;
        case Tag::InvertedSectionStart: {
            Tag endTag = findEndTag(_template, tag, endPos);
            if (endTag.type == Tag::Null) {
                if (m_errorPos == -1) {
                    setError(u"No matching end tag found for inverted section"_s, tag.start);
                }
            } else {
                if (context->isFalse(tag.key)) {
                    output += render(_template, tag.end, endTag.start, context);
                }
                lastTagEnd = endTag.end;
            }
        } break;
        case Tag::SectionEnd:
            setError(u"Unexpected end tag"_s, tag.start);
            lastTagEnd = tag.end;
            break;
        case Tag::Partial: {
            QString tagStartMarker = m_tagStartMarker;
            QString tagEndMarker = m_tagEndMarker;

            m_tagStartMarker = m_defaultTagStartMarker;
            m_tagEndMarker = m_defaultTagEndMarker;

            m_partialStack.push(tag.key);

            QString partialContent = context->partialValue(tag.key);

            // If there is a need to add a special indentation to the partial
            if (tag.indentation > 0) {
                output += u" "_s.repeated(tag.indentation);
                // Indenting the output to keep the parent indentation.
                int posOfLF = partialContent.indexOf(u'\n', 0);
                while (
                    posOfLF > 0 &&
                    posOfLF <
                        (partialContent.length() -
                         1)) { // .length() - 1 because we dont want indentation AFTER the last character if it's a LF
                    partialContent = partialContent.insert(posOfLF + 1, u" "_s.repeated(tag.indentation));
                    posOfLF = partialContent.indexOf(u'\n', posOfLF + 1);
                }
            }

            QString partialRendered = render(partialContent, 0, partialContent.length(), context);

            output += partialRendered;

            lastTagEnd = tag.end;

            m_partialStack.pop();

            m_tagStartMarker = tagStartMarker;
            m_tagEndMarker = tagEndMarker;
        } break;
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

void Renderer::setError(const QString &error, int pos)
{
    Q_ASSERT(!error.isEmpty());
    Q_ASSERT(pos >= 0);

    m_error = error;
    m_errorPos = pos;

    if (!m_partialStack.isEmpty()) {
        m_errorPartial = m_partialStack.top();
    }
}

Tag Renderer::findTag(const QString &content, int pos, int endPos)
{
    int tagStartPos = content.indexOf(m_tagStartMarker, pos);
    if (tagStartPos == -1 || tagStartPos >= endPos) {
        return {};
    }

    int tagEndPos = content.indexOf(m_tagEndMarker, tagStartPos + m_tagStartMarker.length());
    if (tagEndPos == -1) {
        return {};
    }
    tagEndPos += m_tagEndMarker.length();

    Tag tag;
    tag.type = Tag::Value;
    tag.start = tagStartPos;
    tag.end = tagEndPos;
    tag.escapeMode = m_defaultEscapeMode;

    pos = tagStartPos + m_tagStartMarker.length();
    endPos = tagEndPos - m_tagEndMarker.length();

    QChar typeChar = content.at(pos);

    if (typeChar == u'#') {
        tag.type = Tag::SectionStart;
        tag.key = readTagName(content, pos + 1, endPos);
    } else if (typeChar == u'^') {
        tag.type = Tag::InvertedSectionStart;
        tag.key = readTagName(content, pos + 1, endPos);
    } else if (typeChar == u'/') {
        tag.type = Tag::SectionEnd;
        tag.key = readTagName(content, pos + 1, endPos);
    } else if (typeChar == u'!') {
        tag.type = Tag::Comment;
    } else if (typeChar == u'>') {
        tag.type = Tag::Partial;
        tag.key = readTagName(content, pos + 1, endPos);
    } else if (typeChar == u'=') {
        tag.type = Tag::SetDelimiter;
        readSetDelimiter(content, pos + 1, tagEndPos - m_tagEndMarker.length());
    } else {
        if (typeChar == u'&') {
            tag.escapeMode = Tag::Unescape;
            ++pos;
        } else if (typeChar == u'{') {
            tag.escapeMode = Tag::Raw;
            ++pos;
            int endTache = content.indexOf(u'}', pos);
            if (endTache == tag.end - m_tagEndMarker.length()) {
                ++tag.end;
            } else {
                endPos = endTache;
            }
        }
        tag.type = Tag::Value;
        tag.key = readTagName(content, pos, endPos);
    }

    if (tag.type != Tag::Value) {
        expandTag(tag, content);
    }

    return tag;
}

QString Renderer::readTagName(const QString &content, int pos, int endPos)
{
    while (pos < endPos && content.at(pos).isSpace()) {
        ++pos;
    }
    const int start = pos;
    while (pos < endPos && !content.at(pos).isSpace()) {
        ++pos;
    }
    return content.mid(start, pos - start);
}

void Renderer::readSetDelimiter(const QString &content, int pos, int endPos)
{
    while (pos < endPos && content.at(pos).isSpace()) {
        ++pos;
    }

    int start = pos;
    while (pos < endPos && !content.at(pos).isSpace()) {
        if (content.at(pos) == u'=') {
            setError(u"Custom delimiters may not contain '='."_s, pos);
            return;
        }
        ++pos;
    }
    const QString startMarker = content.mid(start, pos - start);

    while (pos < endPos && content.at(pos).isSpace()) {
        ++pos;
    }

    start = pos;
    while (pos < endPos - 1 && !content.at(pos).isSpace()) {
        if (content.at(pos) == u'=') {
            setError(u"Custom delimiters may not contain '='."_s, pos);
            return;
        }
        ++pos;
    }
    const QString endMarker = content.mid(start, pos - start);

    m_tagStartMarker = startMarker;
    m_tagEndMarker = endMarker;
}

Tag Renderer::findEndTag(const QString &content, const Tag &startTag, int endPos)
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
                    setError(u"Tag start/end key mismatch"_s, nextTag.start);
                    return {};
                }
                return nextTag;
            }
        }
        pos = nextTag.end;
    }

    return {};
}

void Renderer::setTagMarkers(const QString &startMarker, const QString &endMarker)
{
    m_defaultTagStartMarker = startMarker;
    m_defaultTagEndMarker = endMarker;
}

void Renderer::expandTag(Tag &tag, const QString &content)
{
    int start = tag.start;
    int end = tag.end;
    int indentation = 0;

    // Move start to beginning of line.
    while (start > 0 && content.at(start - 1) != u'\n') {
        --start;
        if (!content.at(start).isSpace()) {
            return; // Not standalone.
        } else if (content.at(start).category() == QChar::Separator_Space) {
            // If its an actual "white space" and not a new line, it counts toward indentation.
            ++indentation;
        }
    }

    // Move end to one past end of line.
    while (end <= content.size() && content.at(end - 1) != u'\n') {
        if (end < content.size() && !content.at(end).isSpace()) {
            return; // Not standalone.
        }
        ++end;
    }

    tag.start = start;
    tag.end = end;
    tag.indentation = indentation;
}
