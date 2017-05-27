
Checklist For Releasing a New Version of Tomographer
----------------------------------------------------

**BEFORE RELEASE:**

- [ ] Travis CI -- All tests must pass

- [ ] Make sure the binary releases for the different systems compile and run on
      all required systems

- [ ] Make sure other custom modules compile against the new tomographer module
      version

**PERFORMING THE RELEASE:**

- [ ] Make sure Change Log is up to date, including new **version number** and
  **full release date**; **commit & push** changes
  
- [ ] Prepare **release notes**

- [ ] **Tag** the version

- [ ] **Compile binary releases** for the different systems

- [ ] Update the git tag in a full **github release**, add *release notes*, the
      *source archives*, and the *binary releases*

- [ ] Generate new API documentation (`make doc`)

- [ ] Update web site's `_config.yml`

