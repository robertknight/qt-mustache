#pragma once

/** A wrapper around 'char' to provide a subset of the QChar interface. */
class StlCharAdapter
{
	public:
		StlCharAdapter(char ch)
		: m_ch(ch)
		{}

		bool isSpace() const {
			return m_ch == ' ';
		}

		unsigned short unicode() const {
			return m_ch;
		}

		operator char() const {
			return m_ch;
		}

	private:
		char m_ch;
};

/** A wrapper around a std::string reference to provide
 * a subset of the QString interface.
 */
template <class T>
class StlStringAdapter
{
	public:
		StlStringAdapter(T str)
		: m_str(str)
		{}

		StlCharAdapter at(int pos) const {
			return StlCharAdapter(m_str.at(pos));
		}

		int count() const {
			return static_cast<int>(m_str.size());
		}

		int length() const {
			return count();
		}

		void replace(int start, int length, const std::string& str) {
			m_str.replace(start, length, str);
		}

		void replace(const std::string& find, const std::string& replace) {
			int start = indexOf(find, 0);
			if (start != -1) {
				this->replace(start, find.size(), replace);
			}
		}

		int indexOf(const std::string& str, int start) {
			size_t offset = m_str.find(str, start);
			return offset == std::string::npos ? -1 : static_cast<int>(offset);
		}

		int indexOf(char ch, int start) {
			size_t offset = m_str.find(ch, start);
			return offset == std::string::npos ? -1 : static_cast<int>(offset);
		}

		std::string mid(int pos, int length) {
			return m_str.substr(pos, length);
		}

		std::string midRef(int pos, int length) {
			return mid(pos, length);
		}

	private:
		T m_str;
};

// Functions which return a wrapper around a std::string reference
// providing a subset of the QString API

StlStringAdapter<std::string&> adapt(std::string& str) {
	return StlStringAdapter<std::string&>(str);
}

StlStringAdapter<const std::string&> adapt(const std::string& str) {
	return StlStringAdapter<const std::string&>(str);
}

