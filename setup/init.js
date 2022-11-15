const fs = require('fs');
const fetch = require('node-fetch');
const { TextDecoder, TextEncoder } = require('util');
const { JsSignatureProvider } = require('eosjs/dist/eosjs-jssig');
const { Serialize, Api, JsonRpc } = require('eosjs')
const dotenv = require('dotenv');

/**
 * This scrip contains a bunch of useful action helpers 
 * as well as initiliazation scripts for the Multitenant DHO Software
 * 
 * .env should be used to tweak the behaviour of the script, also to provide private keys
 *  
 */

dotenv.config();

const {
  EOS_ENDPOINT,
  DAO_NAME,
  DAO_TITLE,
  DAO_DESCRIPTION,
  VOTING_DURATION_SEC,
  PERIOD_DURATION_SEC,
  PEG_TOKEN,
  REWARD_TOKEN,
  VOICE_TOKEN,
  REWARD_TO_PEG_RATIO,
  ONBOARDER_ACCOUNT,
  VOTING_ALIGNMENT_X100,
  VOTING_QUORUM_X100,
  PERIOD_COUNT,
  WASM_PATH,
  ABI_PATH,
  DEPLOY_CONTRACT,
  DEPLOY_ACCOUNT,
  CREATE_DAO,
  CLEAN_DHO_CONTRACT,
  INIT_DHO_ROOT,
  EOS_PK_1
} = process.env;

//const eosEndpoint = 'http://127.0.0.1:8888'
const eosEndpoint = EOS_ENDPOINT;

const signatureProvider = new JsSignatureProvider([
  EOS_PK_1,
  // EOS_PK_2
]);

const rpc = new JsonRpc(
  eosEndpoint, { fetch }
);

const api = new Api({
  rpc,
  signatureProvider,
  textDecoder: new TextDecoder(),
  textEncoder: new TextEncoder()
});

let actionAccount = 'gh.hypha';

const mtdhoContract = 'mtdhoxhyphaa'
const daoContract = 'dao.hypha';

let contract = mtdhoContract;

const Types = {
  Int: 'int64',
  String: 'string',
  Checksum: 'checksum256',
  Asset: 'asset',
  Name: 'name',
  TimePoint: 'time_point',
} 

const getItem = (label, value, type=Types.String) => (
  {
    "label": label,
    "value": [
        type,
        value
    ]
  }
)

const getDocuments = async ({ limit, reverse } = { limit: 100, reverse: false }) => {

  let readDocs = [];

  let moreDocs = true;
  let nextKey = undefined;

  while(moreDocs && (limit === -1 || readDocs.length < limit)) {

    const { rows, more, next_key } = await rpc.get_table_rows({
      json: true,               // Get the response as json
      code: contract,      // Contract that we target
      scope: contract,         // Account that owns the data
      table: 'documents',        // Table name
      limit: 100,                // Maximum number of rows that we want to get
      reverse: reverse,           // Optional: Get reversed data
      show_payer: false,          // Optional: Show ram payer
      ...(reverse ? { upper_bound: nextKey } : { lower_bound: nextKey })
    });

    nextKey = next_key;

    moreDocs = more;

    let remaining = limit - readDocs.length;

    if (limit !== -1 && rows.length > remaining) {
      rows.length = remaining;
    }
    
    readDocs = readDocs.concat(rows);
  }

  return readDocs;
}

const getEdgesDate = async ({ limit, reverse }) => {
  let readEdges = [];

  let moreEdges = true;
  let nextKey = undefined;

  while(moreEdges && (limit === -1 || readDocs.length < limit)) {

    const { rows, more, next_key } = await rpc.get_table_rows({
      json: true,               // Get the response as json
      code: contract,      // Contract that we target
      scope: contract,         // Account that owns the data
      table: 'edges',        // Table name
      limit: 50,                // Maximum number of rows that we want to get
      reverse: reverse,           // Optional: Get reversed data
      show_payer: false,          // Optional: Show ram payer
      index_position: 8,
      key_type: 'i64',
      ...(reverse ? { upper_bound: nextKey } : { lower_bound: nextKey })
    });

    nextKey = next_key;

    moreEdges = more;

    let remaining = limit - readEdges.length;

    if (limit !== -1 && rows.length > remaining) {
      rows.length = remaining;
    }
    
    readEdges = readEdges.concat(rows);
  }

  return readEdges;
}

const getEdges = async ({ limit, reverse } = { limit: 100, reverse: false }) => {

  let readDocs = [];

  let moreDocs = true;
  let nextKey = undefined;

  while(moreDocs && (limit === -1 || readDocs.length < limit)) {

    const { rows, more, next_key } = await rpc.get_table_rows({
      json: true,               // Get the response as json
      code: contract,      // Contract that we target
      scope: contract,         // Account that owns the data
      table: 'edges',        // Table name
      limit: 50,                // Maximum number of rows that we want to get
      reverse: reverse,           // Optional: Get reversed data
      show_payer: false,          // Optional: Show ram payer
      ...(reverse ? { upper_bound: nextKey } : { lower_bound: nextKey })
    });

    nextKey = next_key;

    moreDocs = more;

    let remaining = limit - readDocs.length;

    if (limit !== -1 && rows.length > remaining) {
      rows.length = remaining;
    }
    
    readDocs = readDocs.concat(rows);
  }

  return readDocs;
}

const findGroup = (document, groupLabel) => {

  const getGroupLabel = (group) => {
    return group[0]?.value[1];
  }

  return document.content_groups.find((group) => {
    return getGroupLabel(group) === groupLabel;   
  })
}

const findItem = (group, label) => {

  if (group === undefined) {
    return undefined;
  }

  return group.find((item) => {
    return item.label === label;
  })
}

const byTypeFilter = (type) => { 
  return (doc) => {
    let system = findGroup(doc, "system");
    let dtype = findItem(system, "type");
    return dtype.value[1] ===  type;
  }
}

const getDocumentsOfType = (documents, type) => {
  return documents.filter(byTypeFilter(type))
}

const runAction = async (action, data) => {

  //console.log("About to run action:", action, "with data:", JSON.stringify(data));

  return api.transact({
    actions: [{
      account: contract,
      name: action,
      authorization: [{
        actor: actionAccount,
        permission: 'active',
      }],
      data: data,
    }]
  }, {
    blocksBehind: 3,
    expireSeconds: 30,
  });
}

const cleanData = () => {
  return runAction('clean', {});
}

const deleteToken = (asset, contract) => {
  return runAction('deletetok', { asset, contract });
}

const createRoot = () => {
  return runAction('createroot', { notes: 'root' });
}

const createDAO = ({
  dao_name,
  dao_title,
  dao_description,
  voting_duration_sec,
  peg_token,
  voice_token,
  reward_token,
  reward_to_peg_ratio,
  period_duration_sec,
  onboarder_account,
  voting_alignment_x100,
  voting_quorum_x100,
  period_count
}) => {
  return runAction('createdao', {
    config: [[
      getItem('content_group_label', 'details', Types.String),
      getItem('dao_name', dao_name, Types.Name),
      getItem('dao_title', dao_title, Types.String),
      getItem('dao_description', dao_description, Types.String),
      getItem('voting_duration_sec', voting_duration_sec, Types.Int),
      getItem('peg_token', peg_token, Types.Asset),
      getItem('voice_token', voice_token, Types.Asset),
      getItem('reward_token', reward_token, Types.Asset),
      getItem('reward_to_peg_ratio', reward_to_peg_ratio, Types.Asset),
      getItem('period_duration_sec', period_duration_sec, Types.Int),
      getItem('voting_alignment_x100', voting_alignment_x100, Types.Int),
      getItem('voting_quorum_x100', voting_quorum_x100, Types.Int),
      getItem('onboarder_account', onboarder_account, Types.Name),
      getItem('period_count', period_count, Types.Int),
    ]]
  })
}

const fixRole = (role) => {
  return runAction('fixrole', { role });
}

const autoEnroll = (dao_hash, enroller, member) => {
  return runAction('autoenroll', {
    dao_hash,
    enroller,
    member
  })
}

const createProposal = (dao_hash, proposer, proposal_type, content_groups) => {
  return runAction('propose', {
    dao_hash, proposer, proposal_type, content_groups
  })
};

const proposePayout = (dao, proposer, {
  recipient,
  title,
  description,
  voice_amount,
  reward_amount,
  peg_amount
}) => {
  return createProposal(
    dao, 
    proposer,
    'payout',
    [[
      getItem('content_group_label', 'details', Types.String),
      getItem('title', title, Types.String),
      getItem('description', description, Types.String),
      getItem('recipient', recipient, Types.Name),
      getItem('voice_amount', voice_amount, Types.Asset),
      getItem('reward_amount', reward_amount, Types.Asset),
      getItem('peg_amount', peg_amount, Types.Asset),
    ]]
  )
}

const proposeBadge = (dao, proposer, { 
  title, 
  description, 
  icon, 
  voice_coefficient_x10000,
  reward_coefficient_x10000,
  peg_coefficient_x10000
 }) => {
   return createProposal(
     dao, 
     proposer,
     'badge',
     [[
      getItem('content_group_label', 'details', Types.String),
      getItem('title', title, Types.String),
      getItem('description', description, Types.String),
      getItem('icon', icon, Types.String),
      getItem('voice_coefficient_x10000', voice_coefficient_x10000, Types.Int),
      getItem('reward_coefficient_x10000', reward_coefficient_x10000, Types.Int),
      getItem('peg_coefficient_x10000', peg_coefficient_x10000, Types.Int)
    ]]
   )
 }

const proposeAssignmet = (dao, proposer, {
  title,
  description,
  assignee,
  role,
  start_period,
  period_count,
  time_share_x100,
  deferred_perc_x100
}) => {
  return createProposal(
    dao, 
    proposer, 
    'assignment', 
    [[
      getItem('content_group_label', 'details', Types.String),
      getItem('title', title, Types.String),
      getItem('description', description, Types.String),
      getItem('assignee', assignee, Types.Name),
      getItem('role', role, Types.Checksum),
      getItem('start_period', start_period, Types.Checksum),
      getItem('period_count', period_count, Types.Int),
      getItem('time_share_x100', time_share_x100, Types.Int),
      getItem('deferred_perc_x100', deferred_perc_x100, Types.Int)
    ]]
  );
}

const getRoleData = ({
  title,
  description,
  min_deferred_x100,
  min_time_share_x100,
  annual_usd_salary
}) => {
  return [[
    getItem('content_group_label', 'details', Types.String),
    getItem('title', title, Types.String),
    getItem('description', description, Types.String),
    getItem('min_deferred_x100', min_deferred_x100, Types.Int),
    getItem('min_time_share_x100', min_time_share_x100, Types.Int),
    getItem('annual_usd_salary', annual_usd_salary, Types.Asset)
  ]];
}

const proposeRole = (dao, proposer, role) => {
  return createProposal(
    dao, 
    proposer, 
    'role', 
    getRoleData(role)
  );
}

const voteProposal = (voter, proposal_hash, vote, notes) => {
  return runAction('vote', {
    voter, proposal_hash, vote, notes: notes ?? " "
  })
}

const closeProposal = (proposal_hash) => {
  return runAction('closedocprop', { proposal_hash });
}

const bummers = {
  dao_name: 'testers',
  dao_title: 'Testers DAO',
  dao_description: 'The best dao in the test west',
  voting_duration_sec: 30,
  period_duration_sec: 60 * 5,
  peg_token: '1.00 PEG',
  reward_token: '1.00 REWARD',
  voice_token: '1.00 VOICE',
  reward_to_peg_ratio: '10.00 PEG',
  onboarder_account: 'gh.hypha',
  voting_alignment_x100: 80,
  voting_quorum_x100: 20,
  period_count: 24
}

const modDoc = (doc, group, value) => {
  return runAction('moddoc', {
    doc,
    group,
    value
  });
}

const createBMDAO = (dao) => {
  return createDAO(dao);
}

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

const createAcc = async (account, pub_key) => {
  const result = await api.transact(
    {
      actions: [
        {
          account: 'eosio',
          name: 'newaccount',
          authorization: [{
            actor: 'eosio',
            permission: 'active',
          }],
          data: {
            creator: 'eosio',
            name: account,
            owner: {
              threshold: 1,
              keys: [{
                key: pub_key,
                weight: 1
              }],
              accounts: [],
              waits: []
            },
            active: {
              threshold: 1,
              keys: [{
                key: pub_key,
                weight: 1
              }],
              accounts: [],
              waits: []
            },
          }
        }
      ]
      
    },
    {
      blocksBehind: 3,
      expireSeconds: 30,
    }
  )

  return result;
}

const setSetting = async (setting, value) => {
  return runAction('setsetting', { key: setting, value, group: "settings" });
}

const sleepFor = async (ms) => {
  console.log("Sleeping for:", ms/1000, "seconds");
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms);
  })
}

const proposeAndPass = async (dao, proposal, type, voteDuration = 35000) => {
  
  let result; 

  switch (type) {
    case 'role':
      result = await proposeRole(dao.hash, actionAccount, proposal);
      break;    
    case 'assignment':
      result = await proposeAssignmet(dao.hash, actionAccount, proposal);
    break;
    case 'payout':
      result = await proposePayout(dao.hash, actionAccount, proposal);
    break;
    case 'badge':
      result = await proposeBadge(dao.hash, actionAccount, proposal);
    break;
    default:
        throw new Error("Invalid proposal type");
      break;
  }

  console.log("Proposing", type, "result:", result, "\nwaiting...");

  await sleepFor(5000);

  const toProposal = await getLastEdgeFrom(dao.id, 'proposal');

  let proposalDoc = await getDocByID(toProposal.to_node);

  console.log("Voting on proposal:", proposalDoc);

  result = await voteProposal(actionAccount, proposalDoc.hash, "pass");

  console.log("Vote result:", result);

  await sleepFor(voteDuration);

  result = await closeProposal(proposalDoc.hash);

  console.log("Close result:", result);

  await sleepFor(1500);

  return await getLastOf(type);
}

const getLastDoc = async () => {
  let docs = await getDocuments({limit: 1, reverse: true });

  return docs[0];
}

const getDocByID = async (id) => {
  const { rows, more, next_key } = await rpc.get_table_rows({
    json: true,               // Get the response as json
    code: contract,      // Contract that we target
    scope: contract,         // Account that owns the data
    table: 'documents',        // Table name
    limit: 1,                // Maximum number of rows that we want to get
    reverse: false,           // Optional: Get reversed data
    show_payer: false,          // Optional: Show ram payer
    lower_bound: id
  });

  return rows[0];
}

const getLastEdgeFrom = async (from, name) => {
  let edges = await getEdgesDate({ limit: -1, reverse: true });

  return edges.find((edge) => {

    return edge.from_node === from && edge.edge_name === name;
  });
}

const getLastOf = async (type, docs) => {
  docs = docs !== undefined ? docs : await getDocuments({limit: -1, reverse: true });
  console.log("docs loaded", docs.length);
  return docs.find(byTypeFilter(type));
}

const testRole = {
  title: 'New Test Role',
  description: 'Testing role proposal',
  annual_usd_salary: '120000.00 USD',
  min_deferred_x100: 50,
  min_time_share_x100: 80,
};

const getTestPayout = (pegPrecision, pegSym, voicePrecision, voiceSym, rewardPrecision, rewardSym) => ({
  recipient: 'gh.hypha',
  title: 'Test Payout',
  description: 'This is a test payout',
  voice_amount: '100.' + '0'.repeat(voicePrecision) + ' ' + voiceSym,
  reward_amount: '50.' + '0'.repeat(rewardPrecision) + ' ' + rewardSym,
  peg_amount: '25.' + '0'.repeat(pegPrecision) + ' ' + pegSym
})

const testBadge = {
  title: 'Test Badge', 
  description: 'This is a test badge', 
  icon: "https://assets.hypha.earth/badges/badge_tech_support.png", 
  voice_coefficient_x10000: 10000,
  reward_coefficient_x10000: 10500,
  peg_coefficient_x10000: 10100
}

const getTestAssignment = (role, start_period, assignee) => ({
  title: "The Best assignment in the wild west",
  description: "Testing assignments",
  assignee,
  role,
  start_period,
  period_count: 10,
  time_share_x100: 100,
  deferred_perc_x100: 60
})

const getAssetSymb = (asset) => {
  return asset.split(' ')[1];
}

const getAssetPrecision = (asset) => {
  return asset.split(' ')[0].split(".")[1].length;
}

const setupDao = async (daoData) => {

  //onboarder_account
  actionAccount = daoData.onboarder_account;

  console.log("Creating dao:", daoData);

  result = await createDAO(daoData);

  console.log("Creation", daoData.dao_name, "result:", result);

  await sleepFor(5000);
  
  let daos = getDocumentsOfType(await getDocuments(), 'dao');

  let dao = daos.find((d) => {
    let details = findGroup(d, "details");
    let name = findItem(details, "dao_name");
    return name.value[1] ===  daoData.dao_name;
  });

  console.log("Dao doc:", { id: dao.id, hash: dao.hash, cgs: JSON.stringify(dao.content_groups)});

  if (dao === undefined) throw new Error("Dao " + daoData.dao_name + " not found");

  //TODO: Gen periods

  const badge = await proposeAndPass(
    dao, 
    testBadge,
    'badge'
  );

  console.log("Badge:", badge);

  const role = await proposeAndPass(dao, testRole, 'role');
  
  console.log("Role:", role);
  
  const toStart = await getLastEdgeFrom(dao.id, 'start');

  let startPeriod = await getDocByID(toStart.to_node);

  const assignment = await proposeAndPass(
    dao, 
    getTestAssignment(role.hash, startPeriod.hash, actionAccount),
    'assignment'
  );

  console.log("Assignment:", assignment);

  const { peg_token, voice_token, reward_token } = daoData;

  const payout = await proposeAndPass(
    dao,
    getTestPayout(
      getAssetPrecision(peg_token), getAssetSymb(peg_token),
      getAssetPrecision(voice_token), getAssetSymb(voice_token), 
      getAssetPrecision(reward_token), getAssetSymb(reward_token)
    ),
    'payout'
  )

  console.log("Payout:", payout);
}

const initializeDHO = async () => {

    let result = await createRoot();
    
    console.log('Create root result:', result);  

    await sleepFor(5000);

    result = await setSetting('governance_token_contract', [Types.Name, 'voice.hypha']);
    
    console.log('Set DHO setting result:', result);

    result = await setSetting('reward_token_contract', [Types.Name, 'hypha.hypha']);
    
    console.log('Set DHO setting result:', result);

    result = await setSetting('peg_token_contract', [Types.Name, 'husd.hypha']);
    
    console.log('Set DHO setting result:', result);

    result = await setSetting('treasury_contract', [Types.Name, 'bank.hypha']);
    
    console.log('Set setting result:', result);

    await sleepFor(2000);
}

const saveAllEdges = async (filename) => {
    
    const edges = await getEdges({ limit: -1, reverse: false });

    console.log("Edges:", edges.length);

    let edgesStr = "";

    for (let edge of edges) {
      edgesStr += `${edge.from_node} ${edge.to_node} ${edge.edge_name}\n`
    }

    fs.writeFileSync('edges.txt', edgesStr);
}

const depContract = async () => {

    console.log("Deploying contract...");

    let result = await deployContract(DEPLOY_ACCOUNT, WASM_PATH, ABI_PATH);

    console.log("Deployment result:", result);

    await sleepFor(5000);
}

const main = async () => {
  try {

    //Contract account
    contract = DEPLOY_ACCOUNT;
    actionAccount = DEPLOY_ACCOUNT;

    if (DEPLOY_CONTRACT === "true") {
      await depContract();
    }

    if (CLEAN_DHO_CONTRACT === "true") {
      console.log("Cleaning Data...");
      const result = await cleanData();
      console.log("Cleanup result:", result);
      await sleepFor(5000);
    }

    if (INIT_DHO_ROOT === "true") {
      await initializeDHO();
    }

    //For localnet
    //createAcc(actionAccount, pub_key);
    
    if (CREATE_DAO === "true") {
      console.log("Creating DAO...");
      await setupDao({
        dao_name:DAO_NAME,
        dao_title:DAO_TITLE,
        dao_description:DAO_DESCRIPTION,
        voting_duration_sec:VOTING_DURATION_SEC,
        period_duration_sec:PERIOD_DURATION_SEC,
        peg_token:PEG_TOKEN,
        reward_token:REWARD_TOKEN,
        voice_token:VOICE_TOKEN,
        reward_to_peg_ratio:REWARD_TO_PEG_RATIO,
        onboarder_account:ONBOARDER_ACCOUNT,
        voting_alignment_x100:VOTING_ALIGNMENT_X100,
        voting_quorum_x100:VOTING_QUORUM_X100,
        period_count:PERIOD_COUNT
      });
    }
  }
  catch (err) {
    console.error("Error:", err);
  }
}

main();