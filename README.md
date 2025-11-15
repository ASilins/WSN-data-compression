# Wireless Sensor Network - Data compression mini-project

This is an Aarhus University Wireless Sensor Networks course semesters mini-project. The goal for the project is write lossy or lossless compression algorithm and transmit the data to the sink where the data is reconstructed. The energy consumption is then analysed.

## Cloning the Repository

To get started with the project locally, it is recommended to clone the repository as a submodule for contiki-ng repository locally.

### Recommended steps:
1. Navigate to root of `Contiki-ng` repository
2. Add the submodule and navigate into the project directory:
    ```sh
    git submodule add https://github.com/ASilins/WSN-data-compression.git
    cd WSN-data-compression
    ```
3. Now you're ready to work with the project.

> :information_source: **Note:** \
The project is setup with the assumption that the submodule is pulled in the root of the `Contiki-ng` project. If setup differently, change contiki location in `Makefile`

## Mote setup
To run the motes you have to either run on Cooja or on two motes. On one mote you have to upload sink code and on the other producer code. You can build the files by running these commands:
1. ```sh
    make TARGET=sky sink.upload
   ```
2. ```sh
    make TARGET=sky producer.upload
   ```

Motes have build configurations. This is what you can add to build:
1. CLASS - Defines if youre building producer or sink. Default is producer. Simple example
```sh
    make TARGET=sky CLASS=sink sink.upload
   ```
2. ALGO - Defines which algorithm you're using. Default sprintz. Simple example
```sh
    make TARGET=sky ALGO=sprintz sink.upload
   ```

## Creating new algorithm
To create a new algorithm you have to make the algorithm code in its own directory. In compression directory update encoder and decoder following the Sprintz algorithms
implementation example. After that, update the Makefile. First add the general build files needed for the algorithm and then add the decode and encode files based on if you're
building the sink or producer.

## Commit messages

It is recommended to follow the following structure for commit messages:
- `add: <commit message>` - used when adding new code/files
- `fix: <commit message>` - used for bug fixes
- `update: <commit message>` - used for update, changes or code optimisation
- `remove: <commit message>` - used for removing code/files
- `refactor: <commit message>` - used when refactoring the code
- `doc: <commit message>` - for documentation changes
- `style: <commit message>` - for formatting, white-space, or style changes

If commit contains more changes and it is hard to group into one, choose on that best describes the changes with the appropriate commit message.

#### Example:
```sh
git commit -m "add: lossy compression algorithm"
```

## Pull Requests

To be able to merge code into develop branch they have to be merged from a Pull Request.

To do that, first create a branch from the develop branch following similar naming strategy as for commit messages:
- `add/<branch_name>`
- `fix/<branch_name>`
- `update/<branch_name>`
- `remove/<branch_name>`
- `refactor/<branch_name>`
- `doc/<branch_name>`
- `style/<branch_name>`

When code changes have been made, push the branch to remote and create a pull request. It should be possible to merge after the Pull Request is approved by at least one person.