var config = {
    apiUrl: "http://127.0.0.1:1984/",
    testnet: false,
    coinUnitPlaces: 12,
    txMinConfirms: 10,
    coinSymbol: 'XMR',
    openAliasPrefix: "xmr",
    coinName: 'Monero',
    coinUriPrefix: 'monero:',
    addressPrefix: 18,
    integratedAddressPrefix: 19,
    addressPrefixTestnet: 53,
    integratedAddressPrefixTestnet: 54,
    feePerKB: new JSBigInt('2000000000'),//10^10
    dustThreshold: new JSBigInt('1000000000'),//10^10 used for choosing outputs/change - we decompose all the way down if the receiver wants now regardless of threshold
    txChargeRatio: 0.5,
    defaultMixin: 3,
    txChargeAddress: '',
    idleTimeout: 30,
    idleWarningDuration: 20,
    maxBlockNumber: 500000000,
    avgBlockTime: 60,
    debugMode: false
};
