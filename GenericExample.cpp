#include <array>
#include <tuple>
#include <vector>
#include <string>
#include <variant>
#include <utility>
#include <concepts>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <functional>
#include <type_traits>


class Dog
{
public:
	Dog(std::string name, int age) :name(std::move(name)), age(age) {}
	void Bark() { std::cout << "汪汪" << std::endl; }
	std::string GetName() { return name; }
	int GetAge() { return age; }

	std::string name;
	int age;
};

class Cat
{
public:
	Cat(std::string name, int age) :name(std::move(name)), age(age) {}
	void Bark() { std::cout << "喵喵" << std::endl; }
	std::string GetName() { return name; }
	int GetAge() { return age; }

	std::string name;
	int age;
};

template<typename T>
concept ANIMAL = requires(T animal)
{
	{animal.GetName()}->std::same_as<std::string>;
	animal.Bark();
	animal.age += 0;
};

template<typename... Rest> requires (ANIMAL<Rest>&&...)
using Animal = std::variant<Rest...>;

template<std::size_t Target, typename Tuple>
	requires (std::tuple_size_v<Tuple> > 0 && Target < std::tuple_size_v<Tuple>) &&
	requires(Tuple& tuple) { std::get<Target>(tuple); std::tuple_cat(tuple, tuple); }
auto tuple_remove(Tuple&& tuple)
{
	return tuple_remove_implement<Target>(std::tuple<>(), std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<Tuple>>());
}

template<std::size_t Target, typename ResultTuple, typename Tuple, std::size_t Index, std::size_t... Rest>
auto tuple_remove_implement(ResultTuple&& result, Tuple&& tuple, std::index_sequence<Index, Rest...>)
{
	if constexpr (Target != Index)
	{
		auto intermediate_result = std::tuple_cat(std::move(result), std::make_tuple<std::tuple_element_t<Index, Tuple>>(std::get<Index>(std::forward<Tuple>(tuple))));
		return tuple_remove_implement<Target>(intermediate_result, std::forward<Tuple>(tuple), std::index_sequence<Rest...>());
	}
	else
		return tuple_remove_implement<Target>(std::move(result), std::forward<Tuple>(tuple), std::index_sequence<Rest...>());
}

template<std::size_t Target, typename ResultTuple, typename Tuple, std::size_t Index>
auto tuple_remove_implement(ResultTuple&& result, Tuple&& tuple, std::index_sequence<Index>)
{
	if constexpr (Target != Index)
		return std::tuple_cat(std::move(result), std::make_tuple<std::tuple_element_t<Index, Tuple>>(std::get<Index>(std::forward<Tuple>(tuple))));
	else
		return std::move(result);
}


template<typename Tuple>
	requires requires(Tuple& tuple) { std::get<0>(tuple); }
class IterateTupleElement
{
	Tuple& tuple;
public:
	IterateTupleElement(Tuple& tuple) :tuple(tuple) {}

	template<typename Callable, typename Result = std::invoke_result_t<Callable, decltype(std::get<0>(tuple))>,
		bool is_void = std::is_same_v<void, Result>, typename Allow = std::enable_if_t<is_void> >
		requires requires(Callable callable, Tuple& tuple) { callable(std::get<0>(tuple)); }
	void foreach(Callable callable)
	{
		foreach(callable, std::make_index_sequence<std::tuple_size_v<Tuple>>());
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
	template<typename Callable, std::size_t Index, std::size_t... Rest>
	void foreach(Callable callable, std::index_sequence<Index, Rest...>)
	{
		callable(std::get<Index>(tuple));
		foreach(callable, std::index_sequence<Rest...>());
	}

	template<typename Callable, std::size_t Index>
	void foreach(Callable callable, std::index_sequence<Index>)
	{
		callable(std::get<Index>(tuple));
	}

	template<typename Result, std::size_t Size, typename Callable, std::size_t Index, std::size_t... Rest>
	void map(std::array<Result, Size>& results, Callable callable, std::index_sequence<Index, Rest...>)
	{
		results[Index] = callable(std::get<Index>(tuple));
		map(results, callable, std::index_sequence<Rest...>());
	}

	template<typename Result, std::size_t Size, typename Callable, std::size_t Index>
	void map(std::array<Result, Size>& results, Callable callable, std::index_sequence<Index>)
	{
		results[Index] = callable(std::get<Index>(tuple));
	}
};

int main()
{
	auto container = std::make_tuple<Dog>({ "alice", 2 });
	{
		auto& container_reference = container;
		auto container = std::tuple_cat(std::move(container_reference), std::make_tuple<Cat>({ "bob", 3 }));
		{
			auto& container_reference = container;
			auto container = std::tuple_cat(std::move(container_reference), std::make_tuple<Dog>({ "charlie", 5 }));

			IterateTupleElement iterator(container);
			iterator.foreach([](auto&& animal) {animal.Bark(); });

			std::cout << "Names: ";
			for (auto&& name : iterator.map([](auto&& animal) {return animal.GetName(); }))
				*std::ostream_iterator<std::string>(std::cout, " ")++ = name;

			{
				auto& container_reference = container;
				auto container = tuple_remove<2>(std::move(container_reference));
				std::cout << "\nAlive animal names: ";
				IterateTupleElement iterator(container);
				for (auto&& name : iterator.map([](auto&& animal) {return animal.GetName(); }))
					*std::ostream_iterator<std::string>(std::cout, " ")++ = name;
			}
		}
	}

	std::cout << "\n--- second way ---\n";
	using Pet = Animal<Dog, Cat>;
	std::vector<Pet> pets;
	pets.emplace_back(Dog{ "alice", 2 });
	pets.emplace_back(Cat{ "bob", 3 });
	pets.emplace_back(Cat{ "charlie", 5 });
	auto call_back = [](auto&& pet) {std::cout << pet.GetName() << ' '; pet.Bark(); };
	auto visit = (void(*)(decltype(call_back)&, Pet&))static_cast<void(*)(decltype(call_back)&&, Pet&&)>(&std::visit<decltype(call_back), Pet>);
	std::ranges::for_each(pets, std::bind(visit, call_back, std::placeholders::_1));
	std::cout << "<<< 5 years later <<<\n";
	for (auto iterator = pets.begin(); iterator != pets.end();)
	{
		if (std::visit([](auto&& pet) {if (pet.age += 5; pet.age > 8)return true; else return false; }, *iterator))
		{
			iterator = pets.erase(iterator);
			continue;
		}
		else
			++iterator;
	}
	std::ranges::for_each(pets, [](auto&& animal) {std::visit([](auto&& pet) {std::cout << pet.GetName() << ' '; pet.Bark(); }, animal); });

	return 0;
}