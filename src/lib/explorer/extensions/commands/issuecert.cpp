/**
 * Copyright (c) 2016-2018 mvs developers
 *
 * This file is part of metaverse-explorer.
 *
 * metaverse-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <metaverse/explorer/json_helper.hpp>
#include <metaverse/explorer/extensions/commands/issuecert.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp>
#include <metaverse/explorer/extensions/base_helper.hpp>

namespace libbitcoin {
namespace explorer {
namespace commands {


console_result issuecert::invoke (Json::Value& jv_output,
    libbitcoin::server::server_node& node)
{
    auto& blockchain = node.chain_impl();
    blockchain.is_account_passwd_valid(auth_.name, auth_.auth);

    blockchain.uppercase_symbol(argument_.symbol);
    boost::to_upper(argument_.cert);

    // check asset symbol
    if (argument_.symbol.length() > ASSET_DETAIL_SYMBOL_FIX_SIZE)
        throw asset_symbol_length_exception{"asset symbol length must be less than 64."};

    // check target address
    if (!blockchain.is_valid_address(argument_.to))
        throw address_invalid_exception{"invalid address parameter! " + argument_.to};

    std::string cert_owner = asset_cert::get_owner_from_address(argument_.to, blockchain);
    if (cert_owner.empty())
        throw did_address_needed_exception("target address is not an did address. " + argument_.to);

    // check asset cert types
    std::map <std::string, asset_cert_type> cert_map = {
        {"DOMAIN_NAMING", asset_cert_ns::domain_naming}
    };
    auto iter = cert_map.find(argument_.cert);
    if (iter == cert_map.end()) {
        throw asset_cert_exception("unknown asset cert type " + argument_.cert);
    }
    auto certs_create = iter->second;

    std::string domain_cert_addr;
    if (certs_create == asset_cert_ns::domain_naming) {
        // check symbol is valid.
        auto pos = argument_.symbol.find(".");
        if (pos == string::npos) {
            throw asset_symbol_name_exception("invalid asset cert symbol " + argument_.symbol
                + ", it should contain a dot '.'");
        }

        auto&& domain = asset_cert::get_domain(argument_.symbol);
        if (!asset_cert::is_valid_domain(domain)) {
            throw asset_symbol_name_exception("invalid asset cert symbol " + argument_.symbol
                + ", it should contain a valid domain!");
        }

        // check domain naming cert not exist.
        if (blockchain.is_asset_cert_exist(argument_.symbol, asset_cert_ns::domain_naming)) {
            throw asset_cert_exception("domain naming cert '" + argument_.symbol + "'already exists!");
        }

        // check domain cert belong to this account.
        auto certs_vec = blockchain.get_account_asset_certs(auth_.name, domain);
        const auto match = [](const business_address_asset_cert& item) {
            return asset_cert::test_certs(item.certs.get_certs(), asset_cert_ns::domain);
        };
        auto it = std::find_if(certs_vec->begin(), certs_vec->end(), match);
        if (it == certs_vec->end()) {
            throw asset_cert_domain_exception("no domain cert owned!");
        }

        domain_cert_addr = it->address;
    }

    // receiver
    std::vector<receiver_record> receiver{
        {argument_.to, argument_.symbol, 0, 0,
            certs_create, utxo_attach_type::asset_cert_issue, attachment()}
    };

    if (certs_create == asset_cert_ns::domain_naming) {
        auto&& domain = asset_cert::get_domain(argument_.symbol);
        receiver.push_back(
            {domain_cert_addr, domain, 0, 0,
                asset_cert_ns::domain, utxo_attach_type::asset_cert, attachment()}
        );
    }

    auto helper = issuing_asset_cert(*this, blockchain,
        std::move(auth_.name), std::move(auth_.auth),
        "", std::move(argument_.symbol),
        std::move(receiver), argument_.fee);

    helper.exec();

    // json output
    auto tx = helper.get_transaction();
    jv_output =  config::json_helper(get_api_version()).prop_tree(tx, true);

    return console_result::okay;
}


} // namespace commands
} // namespace explorer
} // namespace libbitcoin

