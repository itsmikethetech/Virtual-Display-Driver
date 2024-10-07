name: Bug Report
description: File a bug report.
title: "[Bug]: "
labels: ["bug", "triage"]
assignees:

body:
  - type: markdown
    attributes:
      value: |
        Thanks for taking the time to fill out this bug report!
  - type: dropdown
    id: OS
    attributes:
      label: Which OS?
      description: What version of Windows are you running?
      options:
        - Win11 Pro
        - Win11 Home
        - Win10 Pro
        - Win10 Home
        - Other..
      default: 0
    validations:
      required: true
  - type: dropdown
    id: Version
    attributes:
      label: Which release?
      description: What version of Windows are you running?
      options:
        - 24H2
        - 23H2
        - 22H2
        - 21H2
        - 1809
        - 1607
        - 1507
        - Other..
      default: 0
    validations:
      required: true
  - type: textarea
    id: bug-info
    attributes:
      label: Describe the bug
      description: And if you can how to reproduce 
      placeholder: Tell us what you see!
      value: "A clear and concise description of what the bug is.

**Steps to reproduce the behavior:**
1. Go to '...'
2. Click on '....'
3. Scroll down to '....'
4. See error

**Expected behavior**
Description what you expected to happen.
"
    validations:
      required: true 

  - type: input
    id: contact
    attributes:
      label: Contact Details
      description: How can we get in touch with you if we need more info?
      placeholder: ex. email@example.com
    validations:
      required: false

