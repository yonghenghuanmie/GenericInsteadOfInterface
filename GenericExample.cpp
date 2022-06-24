#include <array>
#include <tuple>
#include <iostream>
#include <iterator>
#include <type_traits>

class Dog
{
public:
	Dog(int age) :age(age) {}
	void Bark() { std::cout << "汪汪" << std::endl; }
	int GetAge() { return age; }
private:
	int age;
};

class Cat
{
public:
	Cat(int age) :age(age) {}
	void Bark() { std::cout << "喵喵" << std::endl; }
	int GetAge() { return age; }
private:
	int age;
};

template<typename Tuple>
class IterateTupleElement
{
public:
	IterateTupleElement(Tuple tuple) :tuple(tuple) {}

	template<typename Callable, typename Result = std::invoke_result_t<Callable, decltype(std::get<0>(tuple))>,
		bool is_void = std::is_same_v<void, Result>, typename Allow = std::enable_if_t<is_void> >
		requires requires(Callable callable, Tuple& tuple) { callable(std::get<0>(tuple)); }
	void map(Callable callable)
	{
		map(callable, std::make_index_sequence<std::tuple_size_v<Tuple>>());
	}

	template<typename Callable, typename Result = std::invoke_result_t<Callable, decltype(std::get<0>(tuple))>,
		bool is_void = std::is_same_v<void, Result>, typename Allow = std::enable_if_t<!is_void> >
		requires requires(Callable callable, Tuple& tuple) { callable(std::get<0>(tuple)); }
	auto map(Callable callable, void* overload = nullptr)
	{
		constexpr std::size_t size = std::tuple_size_v<Tuple>;
		std::array<Result, size> results;
		map(results, callable, std::make_index_sequence<size>());
		return results;
	}

private:
	template<typename Result, std::size_t Size, typename Callable, std::size_t Index, std::size_t... Rest>
	auto map(std::array<Result, Size>& results, Callable callable, std::index_sequence<Index, Rest...>)
	{
		results[Index] = callable(std::get<Index>(tuple));
		map(results, callable, std::index_sequence<Rest...>());
	}

	template<typename Result, std::size_t Size, typename Callable, std::size_t Index>
	auto map(std::array<Result, Size>& results, Callable callable, std::index_sequence<Index>)
	{
		results[Index] = callable(std::get<Index>(tuple));
	}

	template<typename Callable, std::size_t Index, std::size_t... Rest>
	void map(Callable callable, std::index_sequence<Index, Rest...>)
	{
		callable(std::get<Index>(tuple));
		map(callable, std::index_sequence<Rest...>());
	}

	template<typename Callable, std::size_t Index>
	void map(Callable callable, std::index_sequence<Index>)
	{
		callable(std::get<Index>(tuple));
	}

	Tuple& tuple;
};


int main()
{
	auto container = std::make_tuple<Dog>(2);
	{
		auto& container_reference = container;
		auto container = std::tuple_cat(container_reference, std::make_tuple<Cat>(3));
		{
			auto& container_reference = container;
			auto container = std::tuple_cat(container_reference, std::make_tuple<Dog>(5));

			IterateTupleElement iterator(container);
			iterator.map([](auto&& animal) {animal.Bark(); });
			std::cout << "Ages: ";
			for (auto&& age : iterator.map([](auto&& animal) {return animal.GetAge(); }))
				*std::ostream_iterator<int>(std::cout, " ")++ = age;
		}
	}
	return 0;
}