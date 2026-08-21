// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <max.kellermann@ionos.com>

#include "Explorer.hxx"
#include "ExplorerListener.hxx"
#include "Error.hxx"
#include "ErrorHandler.hxx"
#include "Client.hxx"
#include "ObjectFlags.hxx"
#include "Resolver.hxx"
#include "net/InetAddress.hxx"
#include "util/Cast.hxx"

#include <avahi-common/error.h>

#include <algorithm>
#include <cassert>

namespace Avahi {

class ServiceExplorer::Object {
	ServiceExplorer &explorer;

	ServiceResolverPtr resolver;

	InetAddress address;

	const ObjectFlags flags;

public:
	explicit Object(ServiceExplorer &_explorer, AvahiLookupResultFlags _flags) noexcept
		:explorer(_explorer),
		 flags{
			 .is_local = (_flags & AVAHI_LOOKUP_RESULT_LOCAL) != 0,
			 .is_our_own = (_flags & AVAHI_LOOKUP_RESULT_OUR_OWN) != 0,
		 }
	{
		address.Clear();
	}

	Object(const Object &) = delete;
	Object &operator=(const Object &) = delete;

	const std::string &GetKey() const noexcept;

	bool IsActive() const noexcept {
		return address.IsDefined();
	}

	bool HasFailed() const noexcept {
		return resolver == nullptr && !IsActive();
	}

	void Resolve(AvahiClient *client, AvahiIfIndex interface,
		     AvahiProtocol protocol,
		     const char *name,
		     const char *type,
		     const char *domain) noexcept;
	void CancelResolve() noexcept;

private:
	void ServiceResolverCallback(AvahiIfIndex interface,
				     AvahiResolverEvent event,
				     const char *host_name,
				     const AvahiAddress *a,
				     uint16_t port,
				     AvahiStringList *txt) noexcept;
	static void ServiceResolverCallback(AvahiServiceResolver *r,
					    AvahiIfIndex interface,
					    AvahiProtocol protocol,
					    AvahiResolverEvent event,
					    const char *name,
					    const char *type,
					    const char *domain,
					    const char *host_name,
					    const AvahiAddress *a,
					    uint16_t port,
					    AvahiStringList *txt,
					    AvahiLookupResultFlags flags,
					    void *userdata) noexcept;
};

inline const std::string &
ServiceExplorer::Object::GetKey() const noexcept
{
	/* this is a kludge which takes advantage of the fact that all
	   instances of this class are inside std::map */
	auto &p = ContainerCast(*this, &Map::value_type::second);
	return p.first;
}

void
ServiceExplorer::Object::Resolve(AvahiClient *client, AvahiIfIndex interface,
				 AvahiProtocol protocol,
				 const char *name,
				 const char *type,
				 const char *domain) noexcept
{
	assert(resolver == nullptr);

	resolver.reset(avahi_service_resolver_new(client, interface, protocol,
						  name, type, domain,
						  /* workaround: the following
						     should be
						     AVAHI_PROTO_UNSPEC
						     (because we can deal with
						     either protocol), but
						     then avahi-daemon
						     sometimes returns IPv6
						     addresses from the cache,
						     even though the service
						     was registered as IPv4
						     only */
						  protocol,
						  AvahiLookupFlags(0),
						  ServiceResolverCallback, this));
	if (resolver == nullptr)
		explorer.error_handler.OnAvahiError(std::make_exception_ptr(MakeError(*client,
										      "Failed to create Avahi service resolver")));
	else
		++explorer.n_resolvers;
}

void
ServiceExplorer::Object::CancelResolve() noexcept
{
	resolver.reset();
}

static constexpr IPv4Address
Import(const AvahiIPv4Address &src, uint_least16_t port) noexcept
{
	return {{src.address}, port};
}

#ifdef HAVE_IPV6

static constexpr IPv6Address
Import(AvahiIfIndex interface, const AvahiIPv6Address &src,
       uint_least16_t port) noexcept
{
	struct in6_addr address;
	static_assert(sizeof(src.address) == sizeof(address), "Wrong size");
	std::copy_n(src.address, sizeof(src.address), address.s6_addr);
	return {address, port, static_cast<uint32_t>(interface)};
}

#endif // HAVE_IPV6

static constexpr InetAddress
Import(AvahiIfIndex interface, const AvahiAddress &src, uint_least16_t port) noexcept
{
	switch (src.proto) {
	case AVAHI_PROTO_INET:
		return Import(src.data.ipv4, port);

#ifdef HAVE_IPV6
	case AVAHI_PROTO_INET6:
		return Import(interface, src.data.ipv6, port);
#else
		(void)interface;
#endif
	}

	InetAddress result;
	result.Clear();
	return result;
}

inline void
ServiceExplorer::Object::ServiceResolverCallback(AvahiIfIndex interface,
						 AvahiResolverEvent event,
						 const char *host_name,
						 const AvahiAddress *a,
						 uint16_t port,
						 AvahiStringList *txt) noexcept
{
	switch (event) {
	case AVAHI_RESOLVER_FOUND:
		address = Import(interface, *a, port);
		explorer.listener.OnAvahiNewObject(GetKey(), host_name, address,
						   txt, flags);
		break;

	case AVAHI_RESOLVER_FAILURE:
		break;
	}

	if (resolver) {
		assert(explorer.n_resolvers > 0);

		resolver.reset();

		if (--explorer.n_resolvers == 0 &&
		    explorer.all_for_now_pending) {
			explorer.all_for_now_pending = false;
			explorer.listener.OnAvahiAllForNow();
		}
	}
}

void
ServiceExplorer::Object::ServiceResolverCallback(AvahiServiceResolver *,
						 AvahiIfIndex interface,
						 [[maybe_unused]] AvahiProtocol protocol,
						 AvahiResolverEvent event,
						 [[maybe_unused]] const char *name,
						 [[maybe_unused]] const char *type,
						 [[maybe_unused]] const char *domain,
						 const char *host_name,
						 const AvahiAddress *a,
						 uint16_t port,
						 AvahiStringList *txt,
						 [[maybe_unused]] AvahiLookupResultFlags flags,
						 void *userdata) noexcept
{
	auto &object = *(ServiceExplorer::Object *)userdata;
	object.ServiceResolverCallback(interface, event, host_name, a, port, txt);
}

ServiceExplorer::ServiceExplorer(Client &_avahi_client,
				 ServiceExplorerListener &_listener,
				 AvahiIfIndex _interface,
				 AvahiProtocol _protocol,
				 const char *_type,
				 const char *_domain,
				 ErrorHandler &_error_handler) noexcept
	:error_handler(_error_handler),
	 avahi_client(_avahi_client),
	 listener(_listener),
	 query_interface(_interface), query_protocol(_protocol),
	 query_type(_type == nullptr ? "" : _type),
	 query_domain(_domain == nullptr ? "" : _domain)
{
	avahi_client.AddListener(*this);
}

ServiceExplorer::~ServiceExplorer() noexcept
{
	avahi_client.RemoveListener(*this);
}

static std::string
MakeKey(AvahiIfIndex interface,
	AvahiProtocol protocol,
	const char *name,
	const char *type,
	const char *domain) noexcept
{
	char buffer[2048];
	snprintf(buffer, sizeof(buffer), "%d/%d/%s/%s/%s",
		 (int)interface, (int)protocol, name, type, domain);
	return buffer;
}

inline void
ServiceExplorer::ServiceBrowserCallback(AvahiServiceBrowser *b,
					AvahiIfIndex interface,
					AvahiProtocol protocol,
					AvahiBrowserEvent event,
					const char *name,
					const char *type,
					const char *domain,
					AvahiLookupResultFlags flags) noexcept
{
	if (event == AVAHI_BROWSER_NEW) {
		auto i = objects.emplace(std::piecewise_construct,
					 std::forward_as_tuple(MakeKey(interface,
								       protocol, name,
								       type, domain)),
					 std::forward_as_tuple(*this, flags));
		if (i.second || i.first->second.HasFailed())
			i.first->second.Resolve(avahi_service_browser_get_client(b),
						interface, protocol,
						name, type, domain);
	} else if (event == AVAHI_BROWSER_REMOVE) {
		auto i = objects.find(MakeKey(interface, protocol, name,
					      type, domain));
		if (i != objects.end()) {
			if (i->second.IsActive())
				listener.OnAvahiRemoveObject(i->first.c_str());

			objects.erase(i);
		}
	} else if (event == AVAHI_BROWSER_ALL_FOR_NOW) {
		if (n_resolvers == 0) {
			assert(!all_for_now_pending);
			listener.OnAvahiAllForNow();
		} else {
			all_for_now_pending = true;
		}
	}
}

void
ServiceExplorer::ServiceBrowserCallback(AvahiServiceBrowser *b,
					AvahiIfIndex interface,
					AvahiProtocol protocol,
					AvahiBrowserEvent event,
					const char *name,
					const char *type,
					const char *domain,
					AvahiLookupResultFlags flags,
					void *userdata) noexcept
{
	auto &cluster = *(ServiceExplorer *)userdata;
	cluster.ServiceBrowserCallback(b, interface, protocol, event, name,
				       type, domain, flags);
}

void
ServiceExplorer::OnAvahiConnect(AvahiClient *client) noexcept
{
	if (avahi_browser != nullptr)
		return;

	avahi_browser.reset(avahi_service_browser_new(client,
						      query_interface, query_protocol,
						      query_type.c_str(),
						      query_domain.empty() ? nullptr : query_domain.c_str(),
						      AvahiLookupFlags(0),
						      ServiceBrowserCallback, this));
	if (avahi_browser == nullptr)
		error_handler.OnAvahiError(std::make_exception_ptr(MakeError(*client,
									     "Failed to create Avahi service browser")));
}

void
ServiceExplorer::OnAvahiDisconnect() noexcept
{
	for (auto &i : objects)
		i.second.CancelResolve();
	n_resolvers = 0;

	avahi_browser.reset();
}

} // namespace Avahi
