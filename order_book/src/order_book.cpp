#include <map>
#include <unordered_map>
#include <string>
#include <atomic>
#include <vector>
template <typename TOrder>
class OrderBook
{
private:
	class SingleOrderBook
	{
	private:
		using IdType = decltype(TOrder::id);
		using SizeType = typename TOrder::Side;
		using StateType = typename TOrder::Status;
		using PriceType = decltype(TOrder::price);
	private:
		using OrderList = std::map<IdType, TOrder*>;
		using Orders = std::unordered_map<IdType, TOrder>;
		using Bids = std::map<PriceType, OrderList, std::greater<PriceType>>;
		using Asks = std::map<PriceType, OrderList, std::less<PriceType>>;
		using id_seed = std::atomic<IdType>;
	private:
		static id_seed _id_seed;
		static IdType invalid_order_id;
	private:
		Orders _odrs;
		Bids _bids;
		Asks _asks;
	private:
		inline IdType new_id()
		{
			return _id_seed.fetch_add(_id_seed, std::memory_order_relaxed);
		}
	public:
		struct matching_record
		{
			TOrder taker;
			TOrder maker;
			matching_record() = default;
			~matching_record() = default;
		};
	public:
		using matching_records = std::vector<matching_record>;
	public:
		SingleOrderBook() = default;
		~SingleOrderBook() = default;
	public:
		auto new_order(TOrder& odr)
		{
			matching_records records;
			if ((odr.side != SizeType::BID && odr.side != SizeType::ASK) || 0 == odr.quantity)
			{
				odr.status = StateType::REJECT;
				return records;
			}
			odr.id = new_id();
			odr.remain_quantity = odr.quantity;
			odr.status = StateType::ACCEPTED;
			if (odr.side == SizeType::BID)
			{
				for (auto it = _asks.begin(); it != _asks.end();)
				{
					auto matched_price = it->first;
					if (matched_price > odr.price)
						break;
					auto list = it->second;
					for (auto it2 = list.begin(); it2 != list.end();)
					{
						auto matching_odr = it2->second;
						auto matched_quantity = odr.remain_quantity < matching_odr->remain_quantity ? odr.remain_quantity : matching_odr->remain_quantity;
						matching_odr->matched_quantity = matched_quantity;
						odr.matched_quantity = matched_quantity;
						matching_odr->matched_price = matched_price;
						odr.matched_price = matched_price;
						matching_odr->matched_id = odr.id;
						odr.matched_id = matching_odr->id;
						matching_odr->remain_quantity -= matched_quantity;
						odr.remain_quantity -= matched_quantity;
						if (0 == matching_odr->remain_quantity)
						{
							matching_odr->status = StateType::FILLED;
						}
						else
						{
							matching_odr->status = StateType::PARTIAL_FILL;
						}
						if (0 == odr.remain_quantity)
						{
							odr.status = StateType::FILLED;
						}
						else
						{
							odr.status = StateType::PARTIAL_FILL;
						}
						matching_record rcd;
						rcd.taker = odr;
						rcd.maker = *matching_odr;
						records.push_back(rcd);
						if (0 == matching_odr->remain_quantity)
						{
							it2 = list.erase(it2);
						}
						else
						{
							++it2;
						}
						if (0 == odr.remain_quantity)
						{
							break;
						}
					}
					if (list.empty())
					{
						it = _asks.erase(it);
					}
					else
					{
						++it;
					}
					if (0 == odr.remain_quantity)
					{
						break;
					}
				}
				if (0 < odr.remain_quantity)
				{
					_odrs[odr.id] = odr;
					auto ptr = &_odrs[odr.id];
					_bids[odr.price][odr.id] = ptr;
				}
			}
			else
			{
				for (auto it = _bids.begin(); it != _bids.end();)
				{
					auto matched_price = it->first;
					if (matched_price < odr.price)
						break;
					auto list = it->second;
					for (auto it2 = list.begin(); it2 != list.end();)
					{
						auto matching_odr = it2->second;
						auto matched_quantity = odr.remain_quantity < matching_odr->remain_quantity ? odr.remain_quantity : matching_odr->remain_quantity;
						matching_odr->matched_quantity = matched_quantity;
						odr.matched_quantity = matched_quantity;
						matching_odr->matched_price = matched_price;
						odr.matched_price = matched_price;
						matching_odr->matched_id = odr.id;
						odr.matched_id = matching_odr->id;
						matching_odr->remain_quantity -= matched_quantity;
						odr.remain_quantity -= matched_quantity;
						if (0 == matching_odr->remain_quantity)
						{
							matching_odr->status = StateType::FILLED;
						}
						else
						{
							matching_odr->status = StateType::PARTIAL_FILL;
						}
						if (0 == odr.remain_quantity)
						{
							odr.status = StateType::FILLED;
						}
						else
						{
							odr.status = StateType::PARTIAL_FILL;
						}
						matching_record rcd;
						rcd.taker = odr;
						rcd.maker = *matching_odr;
						records.push_back(rcd);
						if (0 == matching_odr->remain_quantity)
						{
							it2 = list.erase(it2);
						}
						else
						{
							++it2;
						}
						if (0 == odr.remain_quantity)
						{
							break;
						}
					}
					if (list.empty())
					{
						it = _bids.erase(it);
					}
					else
					{
						++it;
					}
					if (0 == odr.remain_quantity)
					{
						break;
					}
				}
				if (0 < odr.remain_quantity)
				{
					_odrs[odr.id] = odr;
					auto ptr = &_odrs[odr.id];
					_asks[odr.price][odr.id] = ptr;
				}
			}
			return records;
		}
		void cancel_order(TOrder& odr)
		{
			auto id = odr.id;
			auto it = _odrs.find(id);
			if (_odrs.end() == it)
			{
				odr.status = StateType::REJECT;
				return;
			}
			auto ptr = &(it->second);
			auto price = ptr->price;
			auto side = ptr->side;
			if (side == SizeType::BID)
			{
				_bids[price].erase(id);
				if (_bids[price].empty())
				{
					_bids.erase(price);
				}
			}
			else
			{
				_asks[price].erase(id);
				if (_asks[price].empty())
				{
					_asks.erase(price);
				}
			}
			odr.status = StateType::CANCELED;
		}
		auto amend_order(TOrder& odr)
		{
			matching_records records;
			auto id = odr.id;
			auto it = _odrs.find(id);
				return records;
			auto ptr = &(it->second);
			auto price = ptr->price;
			auto side = ptr->side;
			auto quantity = ptr->quantity;
			if (side == odr.side && price == odr.price && quantity >= odr.quantity)
			{
				//do not change priority
				ptr->quantity = odr.quantity;
				return records;
			}
			else
			{
				cancel_order(odr);
				return new_order(odr);
			}
		}
	};
private:
	using SymbolType = decltype(TOrder::symbol);
	using BookMaps = std::unordered_map<SymbolType, SingleOrderBook>;
private:
	BookMaps _maps;
public:
	OrderBook() = default;
	~OrderBook() = default;
public:
	auto new_order(TOrder& odr)
	{
		return _maps[odr.symbol].new_order(odr);
	}
	auto cancel_order(TOrder& odr)
	{
		return _maps[odr.symbol].cancel_order(odr);
	}
	auto amend_order(TOrder& odr)
	{
		return _maps[odr.symbol].amend_order(odr);
	}
};

template <typename TOrder>
typename OrderBook<TOrder>::SingleOrderBook::id_seed OrderBook<TOrder>::SingleOrderBook::_id_seed(0);

template <typename TOrder>
typename OrderBook<TOrder>::SingleOrderBook::IdType OrderBook<TOrder>::SingleOrderBook::invalid_order_id(0);

struct Order
{
	enum Side : unsigned long long
	{
		NONE = 0,
		BID = 1,
		ASK = 2
	};
	enum Status: unsigned long long
	{
		INIT = 0,
		ACCEPTED = 1,
		PARTIAL_FILL = 2,
		FILLED = 3,
		CANCELED = 4,
		REJECT = 5
		
	};
	Side side;
	Status status;
	long long price;
	long long matched_price;
	unsigned long long id;
	unsigned long long client_order_id;
	unsigned long long matched_id;
	unsigned long long quantity;
	unsigned long long remain_quantity;
	unsigned long long matched_quantity;
	std::string symbol;
	Order():
		side(NONE),
		status(INIT),
		price(0),
		matched_price(0),
		id(0),
		client_order_id(0),
		matched_id(0),
		quantity(0),
		remain_quantity(0),
		matched_quantity(0),
		symbol("")
	{
	}
	~Order() = default;
};

int main(void)
{
	OrderBook<Order> book;
	Order odr;
	odr.symbol = "s1";
	odr.price = 100;
	odr.side = Order::Side::BID;
	odr.quantity = 60;
	book.new_order(odr);

	odr.price = 99;
	book.new_order(odr);
	odr.price = 98;
	book.new_order(odr);
	

	odr.side = Order::Side::ASK;
	odr.price = 101;
	book.new_order(odr);

	odr.price = 102;
	book.new_order(odr);

	odr.price = 103;
	book.new_order(odr);

	odr.id = 3;

	book.cancel_order(odr);

	odr.id = 5;
	book.amend_order(odr);

	return 0;
}
