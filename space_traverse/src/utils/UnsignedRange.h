#pragma once
#include <list>
#include <memory>
#include <ostream>

class UnsignedRange
{
public:
	UnsignedRange() {};
	~UnsignedRange() {};

	void Add(size_t number);
	void AddInterval(size_t first, size_t last);
	void RemoveInterval(size_t first, size_t last);

	bool Contains(size_t number) const;
	size_t GetPositionOf(size_t number) const;
	size_t Size() const;

	UnsignedRange operator+(const UnsignedRange& other) const;
	UnsignedRange operator-(const UnsignedRange& other) const;
	friend std::ostream& operator<<(std::ostream& stream, const UnsignedRange& matrix);

private:
	void ResolveCollisions(std::list<std::pair<size_t, size_t>>::iterator start);
	std::list<std::pair<size_t, size_t>> m_data;

	friend class RangeIterator;
};

class RangeIterator
{
public:
	RangeIterator(const UnsignedRange& range) : m_range(range) { Reset(); }

	void Reset()
	{
		m_iter = m_range.m_data.begin();
		m_current = 0;
	}

	bool AtEnd() const { return m_iter == m_range.m_data.end(); }

	size_t Get() const { return m_iter->first + m_current; }

	bool Advance()
	{
		m_current++;

		if (Get() == m_iter->second)
		{
			m_current = 0;
			m_iter++;
		}

		if (m_iter != m_range.m_data.end())
		{
			return true;
		}

		return false;
	}

	bool Advance(size_t& i)
	{
		bool result = Advance();
		i = Get();

		return result;
	}

	size_t GetMax() const { return m_range.m_data.back().second - 1; }

private:
	UnsignedRange m_range;
	int64_t m_current;
	std::list<std::pair<size_t, size_t>>::iterator m_iter;
};
