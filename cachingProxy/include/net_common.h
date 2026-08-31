#pragma once


#include <memory>
#include <thread>
#include <mutex>
#include <optional>
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <list>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <ranges>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00

#endif

#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio/co_spawn.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/awaitable.hpp>

#define ASIO_HAS_OPENSSL

#include <asio/ssl.hpp>

#ifdef _WIN32

#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")
void load_windows_system_certs(asio::ssl::context& ctx) {
	HCERTSTORE hStore = CertOpenSystemStoreA(0, "ROOT");
	if (!hStore) return;

	X509_STORE* store = SSL_CTX_get_cert_store(ctx.native_handle());
	PCCERT_CONTEXT pContext = NULL;

	while ((pContext = CertEnumCertificatesInStore(hStore, pContext)) != NULL) {
		const unsigned char* pbCertEncoded = pContext->pbCertEncoded;
		X509* x509 = d2i_X509(NULL, &pbCertEncoded, pContext->cbCertEncoded);
		if (x509) {
			X509_STORE_add_cert(store, x509);
			X509_free(x509);
		}
	}
	CertCloseStore(hStore, 0);

}

#endif