const deployContract = async (account, wasmPath, abitPath) => {

  const wasm = fs.readFileSync(wasmPath).toString('hex');

  const buffer = new Serialize.SerialBuffer({
    textEncoder: api.textEncoder,
    textDecoder: api.textDecoder,
  })

  let abi = JSON.parse(fs.readFileSync(abitPath, 'utf8'));

  const abiDefinition = api.abiTypes.get('abi_def');

  abi = abiDefinition.fields.reduce(
    (acc, { name: fieldName }) =>
      Object.assign(acc, { [fieldName]: acc[fieldName] || [] }),
    abi
  )

  abiDefinition.serialize(buffer, abi)

  let resultABI;
  let resultWASM;

  try {
    resultABI = await api.transact(
      {
        actions: [
          {
            account: 'eosio',
            name: 'setabi',
            authorization: [
              {
                actor: account,
                permission: 'active',
              },
            ],
            data: {
              account: account,
              abi: Buffer.from(buffer.asUint8Array()).toString(`hex`),
            },
          },
        ],
      },
      {
        blocksBehind: 3,
        expireSeconds: 30,
      }
    )
  }
  catch (error) {
    resultABI = error;
  }

  try {
    resultWASM = await api.transact(
      {
        actions: [
          {
            account: 'eosio',
            name: 'setcode',
            authorization: [
              {
                actor: account,
                permission: 'active',
              },
            ],
            data: {
              vmtype: 0,
              vmversion: 0,
              account: account,
              code: wasm,
            },
          },
        ],
      },
      {
        blocksBehind: 3,
        expireSeconds: 30,
      }
    )
  }
  catch (error) {
    resultWASM = error;
  }

  return { wasm: resultWASM, abi: resultABI };
}
