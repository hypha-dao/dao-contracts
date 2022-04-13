"""
Requires libeospy
"""

from eospy.cleos import Cleos
import json

API_URL = "https://telos.greymass.com"
HYPHA_DAO_NAME = 'dao.hypha'
HVOICE_NAME = 'voice.hypha'
HVOICE_TOKEN = 'HVOICE'

HUSD_NAME = 'husd.hypha'
HUSD_TOKEN = 'HUSD'

HYPHA_NAME = 'hypha.hypha'
HYPHA_TOKEN = 'HYPHA'

BANK_NAME = "bank.hypha"
COSTAK_NAME = "costak.hypha"


def content_group(groups, label):
    for group in groups:
        for content in group:
            if content['label'] == 'content_group_label' and content['value'][1] == label:
                return group

    return None


def content_value(group, label):
    for content in group:
        if content['label'] == label:
            return content['value'][1]

    return None


def fetch_stats(client, contract, token):
    return client.get_table(
        contract,
        token,
        'stat'
    )['rows'][0]


def fetch_values(client, contract, token, members):
    values = []
    for member in members:
        try:
            value = next(filter(
                lambda a: a['balance'].split()[-1] == token,
                client.get_table(
                    contract,
                    member,
                    'accounts'
                )['rows']
            ))
            value['member'] = member
            values.append(value)
        except StopIteration:
            continue
    return values


def exhaust(client, account, scope, table):
    data = []
    cursor = 0
    limit = 100

    while True:
        response = client.get_table(
            account,
            scope,
            table,
            limit=limit,
            lower_bound=cursor
        )

        data.extend(response['rows'])

        if not response['more']:
            break

        cursor = response['next_key']

    return data


def _main():
    client = Cleos(API_URL)

    redemptions = exhaust(client, BANK_NAME, BANK_NAME, 'redemptions')
    payments = exhaust(client, BANK_NAME, BANK_NAME, 'payments')

    balances = exhaust(client, COSTAK_NAME, COSTAK_NAME, 'balances')
    locks = exhaust(client, COSTAK_NAME, COSTAK_NAME, 'locks')

    member_ids = map(
        lambda row: row['to_node'],
        filter(
            lambda row: row['edge_name'] == 'member',
            client.get_table(
                HYPHA_DAO_NAME,
                HYPHA_DAO_NAME,
                'edges',
                index_position=4,
                key_type='name',
                lower_bound='member',
                limit=-1
            )['rows']
        )
    )

    members = []
    for member_id in member_ids:
        result = client.get_table(
            HYPHA_DAO_NAME,
            HYPHA_DAO_NAME,
            'documents',
            lower_bound=member_id,
            limit=1
        )['rows']
        if len(result) == 0 or result[0]['id'] != member_id:
            print(f'Invalid member id found: {member_id}. Found edge but not the document. Skipping')
            continue

        details = content_group(result[0]['content_groups'], 'details')
        member = content_value(details, 'member')
        members.append(member)

    hvoice_stats = fetch_stats(client, HVOICE_NAME, HVOICE_TOKEN)
    hvoice_values = fetch_values(client, HVOICE_NAME, HVOICE_TOKEN, members)

    husd_stats = fetch_stats(client, HUSD_NAME, HUSD_TOKEN)
    husd_values = fetch_values(client, HUSD_NAME, HUSD_TOKEN, members)

    hypha_stats = fetch_stats(client, HYPHA_NAME, HYPHA_TOKEN)
    hypha_values = fetch_values(client, HYPHA_NAME, HYPHA_TOKEN, members)

    with open('bank-redemptions.json', 'w') as file:
        json.dump(redemptions, file, indent=2)

    with open('bank-payments.json', 'w') as file:
        json.dump(payments, file, indent=2)

    with open('costak-balances.json', 'w') as file:
        json.dump(balances, file, indent=2)

    with open('costak-locks.json', 'w') as file:
        json.dump(locks, file, indent=2)

    with open('members.json', 'w') as file:
        json.dump(members, file, indent=2)

    with open('hvoice_stats.json', 'w') as file:
        json.dump(hvoice_stats, file, indent=2)

    with open('hvoice_accounts.json', 'w') as file:
        json.dump(hvoice_values, file, indent=2)

    with open('hypha_stats.json', 'w') as file:
        json.dump(hypha_stats, file, indent=2)

    with open('hypha_accounts.json', 'w') as file:
        json.dump(hypha_values, file, indent=2)

    with open('husd_stats.json', 'w') as file:
        json.dump(husd_stats, file, indent=2)

    with open('husd_accounts.json', 'w') as file:
        json.dump(husd_values, file, indent=2)


if __name__ == '__main__':
    _main()
