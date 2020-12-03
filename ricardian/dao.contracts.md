
<h1 class="contract">apply</h1>

---
spec_version: "0.2.0"
title: Apply
summary: 'Apply for Membership'
icon: https://eos.gyftie.org/statics/logos/mobile.png#54b516210a9fe709b1ccc8c075e16538d51e3664674f707f80630a7e53dbd2e7
---

{{$action.authorization.[0].actor}} applies 


<h1 class="contract">closeprop</h1>

---
spec_version: "0.2.0"
title: Close Proposal
summary: 'Close the Proposal after the Voting Period ends'
icon: https://eos.gyftie.org/statics/logos/mobile.png#54b516210a9fe709b1ccc8c075e16538d51e3664674f707f80630a7e53dbd2e7
---

{{$action.authorization.[0].actor}} applies 

<h1 class="contract">create</h1>

---
spec_version: "0.2.0"
title: Create Object
summary: 'Create an object in the contract'
icon: https://eos.gyftie.org/statics/logos/mobile.png#54b516210a9fe709b1ccc8c075e16538d51e3664674f707f80630a7e53dbd2e7
---

{{$action.authorization.[0].actor}} applies 

<h1 class="contract">enroll</h1>

---
spec_version: "0.2.0"
title: Enroll
summary: 'Enroll a member in the organization'
icon: https://eos.gyftie.org/statics/logos/mobile.png#54b516210a9fe709b1ccc8c075e16538d51e3664674f707f80630a7e53dbd2e7
---

{{$action.authorization.[0].actor}} enrolls the existing applicant {{applicant}} with the content {{content}} to Hypha DAO." 

<h1 class="contract">payassign</h1>

---
spec_version: "0.2.0"
title: Claim Assignment Pay
summary: 'Claim assignment pay for a specified assignment and period ID'
icon: https://eos.gyftie.org/statics/logos/mobile.png#54b516210a9fe709b1ccc8c075e16538d51e3664674f707f80630a7e53dbd2e7
---

{{$action.authorization.[0].actor}} pays the given assignment with the id: {{assignment_id}} in the period with the id: {{period_id}}."
