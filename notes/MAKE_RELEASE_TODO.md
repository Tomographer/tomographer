
Checklist For Releasing a New Version of Tomographer
----------------------------------------------------

**BEFORE RELEASE:**

- [ ] Travis CI -- All tests must pass

- [ ] Make sure the binary releases for the different systems compile and run on
      all required systems

- [ ] Make sure other custom modules compile against the new tomographer module
      version
      
- [ ] Make sure `python setup.py sdist` and `python setup.py bdist_wheel` work
      for both `python2` and `python3`; if changes were made to the setup file,
      upload and test via TestPyPi first.

**PERFORMING THE RELEASE:**

- [ ] Make sure Change Log is up to date, including new **version number** and
  **full release date**; **commit & push** changes
  
- [ ] Prepare **release notes**

- [ ] **Tag** the version

- [ ] **Compile binary releases** for the different systems

- [ ] Update the git tag in a full **github release**, add *release notes*, the
      *source archives*, and the *binary releases*

- [ ] Generate new API documentation (`make doc`), copy to web site tree

- [ ] Update web site's `_config.yml`

- [ ] Generate source Python package (NOT WHEEL) via `python3 setup.py sdist`;
      upload it to PyPI.
      
- [ ] Inspect and clean up Zenodo entry (click on DOI to get there)

