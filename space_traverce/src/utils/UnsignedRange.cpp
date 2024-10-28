#include "UnsignedRange.h"
#include <stdexcept>

void UnsignedRange::Add(size_t number)
{
	// expand ranges
	for (auto iterator = m_data.begin(); iterator != m_data.end(); iterator++)
	{

		if (number > iterator->second)
		{
			// check next range
			continue;
		}
		else if (number == iterator->second)
		{
			// expand right
			iterator->second = number + 1;

			//check if ranges overlap
			iterator++;
			std::list<std::pair<size_t, size_t>>::iterator next = iterator;
			iterator--;

			if (next != m_data.end())
			{
				if (next->first == number + 1)
				{
					// combine ranges
					next->first = iterator->first;
					m_data.erase(iterator);
				}
			}

			return;
		}
		else if (number < iterator->first)
		{
			if (iterator->first - number == 1)
			{
				// expand left
				iterator->first = number;

				//check if ranges overlap
				if (iterator != m_data.begin())
				{
					iterator--;
					std::list<std::pair<size_t, size_t>>::iterator prev = iterator;
					iterator++;

					if (prev->second == number)
					{
						// combine ranges
						prev->second = iterator->second;
						m_data.erase(iterator);
					}
				}
				return;
			}
			else
			{
				// create new range left
				m_data.insert(iterator, { number, number + 1 });
				return;
			}
		}
		else
		{
			// number is already in range
			return;
		}
	} // ranges loop

	// create new range right
	m_data.push_back({ number, number + 1 });
}

void UnsignedRange::AddInterval(size_t first, size_t last)
{
	// first must be bigger than last
	if (first > last)
	{
		std::swap(first, last);
	}

	for (auto iterator = m_data.begin(); iterator != m_data.end(); iterator++)
	{
		// FIRST is to the left of current range
		if (first < iterator->first)
		{
			// SECOND is to the left of a curent range
			if (last < iterator->first)
			{
				// insert new range to the left of current
				m_data.insert(iterator, { first, last + 1 });
				return;
			}
			// SECOND is inside or to the left of current range
			else
			{
				// expand current range to the left
				iterator->first = first;

				// if SECOND is to the right of current range
				if (last + 1 > iterator->second)
				{
					// expand current range to the right
					iterator->second = last + 1;

					// resolve collisions
					ResolveCollisions(iterator);
				}

				return;
			}
		}
		// FIRST is inside of the current range
		else if (first <= iterator->second)
		{
			// SECOND is to the right of current range
			if (last + 1 > iterator->second)
			{
				// expand current range to the right
				iterator->second = last + 1;

				// resolve collisions
				ResolveCollisions(iterator);
			}
			// else SECOND is inide of the current range

			return;

		}
		// FIRST is to the right of the current range
		else
		{
			// procede to the next range
			continue;
		}


	} //loop over ranges

	// FIRST is to the right of the last range
	// insert new range in the end position
	m_data.insert(m_data.end(), { first, last + 1 });
}

void UnsignedRange::RemoveInterval(size_t first, size_t last)
{
	std::list<std::pair<size_t, size_t>>::iterator delBegin = m_data.begin();
	std::list<std::pair<size_t, size_t>>::iterator delEnd = m_data.begin();

	for (auto iterator = m_data.begin(); iterator != m_data.end(); iterator++)
	{
		// FIRST is to the left of the current range
		if (first < iterator->first + 1)
		{

			// LAST is to the left of the current range
			if (last < iterator->first)
			{
				// nothing to remove
				break;
			}
			// LAST is inside of the current range
			else if (last < iterator->second - 1)
			{
				// cut current range
				iterator->first = last + 1;

				break;
			}
			// LAST is to the right of the current range
			else
			{
				// mark current range for deletion
				delEnd++;
			}

		}
		// FIRST is inside of the current range
		else if (first < iterator->second)
		{

			// LAST is inside of a current range
			if (last < iterator->second - 1)
			{
				// split current range into two
				auto tmp = iterator->second;
				iterator->second = first;

				iterator++;
				m_data.insert(iterator, { last + 1, tmp });

				break;
			}
			// LAST is to the right of the current range
			else
			{
				// cut current range
				iterator->second = first;

				delBegin++;
				delEnd++;
			}
		}
		// first is to the right of the current range
		else
		{
			delBegin++;
			delEnd++;
		}
	} // loop over ranges

	m_data.erase(delBegin, delEnd);
}

bool UnsignedRange::IsInRange(size_t number) const
{
	for (auto& range : m_data)
	{
		if (number >= range.first && number < range.second)
		{
			return true;
		}
	}
	return false;
}

size_t UnsignedRange::GetPositionOf(size_t number) const
{
	if (!IsInRange(number))
	{
		throw std::out_of_range("Argument is out of range.");
	}

	size_t result = 0;

	for (auto iterator = m_data.begin(); iterator != m_data.end(); iterator++)
	{
		if (number > iterator->second)
		{
			result += iterator->second - iterator->first;
		}
		else
		{
			result += number - iterator->first;
			break;
		}

	}

	return result;
}

size_t UnsignedRange::Size() const
{
	size_t result = 0;
	
	for (auto iterator = m_data.begin(); iterator != m_data.end(); iterator++)
	{
		result += iterator->second - iterator->first;
	}

	return result;
}

void UnsignedRange::ResolveCollisions(std::list<std::pair<size_t, size_t>>::iterator start)
{
	if (m_data.size() < 2)
	{
		return;
	}

	std::list<std::pair<size_t, size_t>>::iterator delBegin = start;
	std::list<std::pair<size_t, size_t>>::iterator delEnd = start;
	delBegin++;

	auto iterator = delBegin;

	while (iterator != m_data.end())
	{
		if (start->second < iterator->first)
		{
			// no collisions
			return;
		}
		else if (start->second <= iterator->second)
		{
			// expand start to the right
			start->second = iterator->second;

			// remove iterator 
			delEnd = iterator;
			break;
		}
		else
		{
			// range is consumed
			// remove iterator
			delEnd = iterator;
		}

		// advance
		iterator++;
	}

	delEnd++;
	m_data.erase(delBegin, delEnd);
}

std::ostream& operator<<(std::ostream& stream, const UnsignedRange& matrix)
{
	for (auto& range : matrix.m_data)
	{
		stream << "[" << range.first << "," << range.second << ") ";
	}

	return stream;
}
