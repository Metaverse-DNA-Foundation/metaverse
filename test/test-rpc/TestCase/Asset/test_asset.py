from TestCase.MVSTestCase import *

class TestAsset(MVSTestCaseBase):
    def test_1_create_asset(self):
        Alice.create_asset(False)
        #validate result
        assets = Alice.get_accountasset()
        self.assertEqual(len(assets), 1)
        self.assertEqual(assets[0].symbol, Alice.asset_symbol)
        self.assertEqual(assets[0].address, "")
        self.assertEqual(assets[0].issuer, Alice.name)
        self.assertEqual(assets[0].status, 'unissued')

        #delete_localasset
        Alice.delete_localasset()
        assets = Alice.get_accountasset()
        self.assertEqual(len(assets), 0)

    def test_2_issue_asset_from(self):
        Alice.create_asset(False)
        Alice.issue_asset(Alice.mainaddress())
        Alice.mining()

        assets = Alice.get_accountasset()
        self.assertEqual(len(assets), 1)
        self.assertEqual(assets[0].symbol, Alice.asset_symbol)
        self.assertEqual(assets[0].address, Alice.mainaddress())
        self.assertEqual(assets[0].issuer, Alice.name)
        self.assertEqual(assets[0].status, 'unspent')

        addressassets = Alice.get_addressasset( Alice.mainaddress() )
        addressasset = filter(lambda a: a.symbol == Alice.asset_symbol, addressassets)
        self.assertEqual(len(addressasset), 1)
        self.assertEqual(addressasset[0].symbol, Alice.asset_symbol)
        self.assertEqual(addressasset[0].address, Alice.mainaddress())
        self.assertEqual(addressasset[0].issuer, Alice.name)
        self.assertEqual(addressasset[0].status, 'unspent')

    def get_asset_amount(self, role):
        addressassets = role.get_addressasset(role.mainaddress())

        #we only consider Alice's Asset
        addressasset = filter(lambda a: a.symbol == Alice.asset_symbol, addressassets)
        self.assertEqual(len(addressasset), 1)
        previous_quantity = addressasset[0].quantity
        previous_decimal = addressasset[0].decimal_number
        return previous_quantity * (10 ** previous_decimal)

    def test_3_sendasset(self):
        origin_amount = self.get_asset_amount(Alice)
        send_amount = 100
        #pre-set condition
        self.assertGreater(origin_amount, send_amount)
        Alice.send_asset(Zac.mainaddress(), send_amount)
        Alice.mining()

        final_amount = self.get_asset_amount(Alice)
        self.assertEqual(origin_amount-send_amount, final_amount)
        self.assertEqual(send_amount, self.get_asset_amount(Zac))

    def test_4_sendassetfrom(self):
        origin_amount = self.get_asset_amount(Alice)
        send_amount = 100
        # pre-set condition
        self.assertGreater(origin_amount, send_amount)
        Alice.send_asset_from(Alice.mainaddress(), Zac.mainaddress(), send_amount)
        Alice.mining()

        final_amount = self.get_asset_amount(Alice)
        self.assertEqual(origin_amount - send_amount, final_amount)
        self.assertEqual(send_amount, self.get_asset_amount(Zac))

    def test_5_burn_asset(self):
        #use the asset created in the previous test case
        #amout > previous_amount
        amount = self.get_asset_amount(Alice)
        ec, message = mvs_rpc.burn(Alice.name, Alice.password, Alice.asset_symbol, amount + 1)
        self.assertEqual(ec, 5001, message)

        Alice.burn_asset(amount-1)
        Alice.mining()

        current_amount = self.get_asset_amount(Alice)
        self.assertEqual(current_amount, 1)

        Alice.burn_asset(1)
        Alice.mining()
        addressassets = Alice.get_addressasset(Alice.mainaddress())
        addressasset = filter(lambda a: a.symbol == Alice.asset_symbol, addressassets)
        self.assertEqual(len(addressasset), 0)







