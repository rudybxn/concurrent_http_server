This is a quick refresher of how to use Git.

## 1. Creating a Branch
All work an individual will do will be off of a Git branch. A Git branch is essentially a copy of the Git project that you are branching off of. So, let's say that you are branching off of the 'main' branch; the branch created will make a copy of the 'main' branch to which you can edit without worrying about breaking code that's already on 'main'. We decided to work off of Git branches to minimize harm done to the 'main' branch which should always be working.

### 1a. How to create a branch on the console/terminal 
Recommended as this is the easiest. 

    i.   On the project repository of you serial console, make sure you're on the `main` branch
    ii.  Do `git pull` to ensure that the branch you're about to create contains the most up-to-date changes off of `main`
    iii. Then do:

```
bash~: git checkout -b 'branch_name'
```

### 1b. How to create a branch from GitHub
By the end of these steps, you will have pulled down the branch that you've created on GitHub.

    i.   Go to the project repository on GitHub (project-webserver-team0101)
    ii.  Click the drop down menu of 'main'
    iii. Click 'View all branches'
    iv.  Click the green 'New branch' button
    v.   Type in the branch name
    vi.  Click the green `Create new branch` button
    vii. *On your serial terminal*, proceed to the project you've cloned
    viii.Make sure you're on the 'main' branch
    ix.  Do `git pull`

## 2. Staging Changes to the Branch
Once you've edited your branch (i.e. edited a pre-existing file, added a new file, etc.), you'll notice that your branch has unsaved changes highlighted in red when you do `git status`. The first step to saving those changes is by "staging" them (getting them ready to be saved). Once you get the "staged," you'll notice that the changes you've made are green when you do `git status` again.

### 2a. How to determine what changes have been made

```
bash~: git status
```

### 2b. How to stage changes

```
bash~: git add <name of file(s)>
```

## 3. Committing Changes to your Branch
A commit is the last thing you need to do before you push up your changes. It is essentially a marker (a hash) to which we can refer back to understand what edits were done at this point in time. You'll notice that if you do a `git status`, you'll no longer see your changes. Don't freak out! Your changes are still there (you'll see your commit if you do `git history`).

```
bash~: git commit -m "<Enter quick blurb of what changes you've made>"
```

## 4. Pushing Changes Up
This is the last step to see your changes reflected back up on GitHub.

To get here, you have: 
1. Created a branch
2. Made changes to the branch
3. Staged your changes
4. Commited your changes

```
bash~: git push
```

Once your changes are up on GitHub, you're ready to create a Pull Request (PR) for everyone to review. Once the review is over, you will have the ability to merge the changes on your branch onto the `main` branch.

## Additional Notes

- Always be sure that your `main` branch on the team repository that you've cloned is up-to-date (this is done by `git pull`)
- To change to different existing branches, simply do `git checkout <branch name>`
- Git can be challenging, so be sure to:
    - 1. Utilize the internet for help
    - 2. Ask teammates for help (feel free to contact me `Tomas`)
    - 3. Ask TAs and teachers for help
        - A mistake made on Git can create a legit sphagetti mess. So, please don't be afraid to ask for help
- Refer to the Google Sheets for the naming convention of Git branches
- For very small changes (i.e. nothing code related like adding this document), I think it'll be okay if you push straight to 'main'
