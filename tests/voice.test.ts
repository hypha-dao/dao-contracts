import { setupEnvironment } from "./setup";
import { Asset } from "./types/Asset";

describe('Voice', () => {
    it('issues and transfers', async () => {
        const environment = await setupEnvironment();
        let issuedVoice = environment.getIssuedHvoice();
        expect(issuedVoice).toEqual(Asset.fromString('104.00 HVOICE'));

        await environment.peerContracts.voice.contract.transfer({
            from: environment.members[1].account.accountName,
            to: environment.members[0].account.accountName,
            quantity: '50.00 HVOICE',
            memo: 'hvoice transfer'
        });

    });
});
